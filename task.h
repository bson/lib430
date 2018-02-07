#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>
#include "common.h"

// Simple task abstraction.  Its main purpose is to support a simple
// model where there is a single main or "idle" context, and additional
// service tasks for e.g. USB or I2C Device support to do processing too
// complex for an ISR.  In this case the ISR notes work to be done, e.g.
// by registering an event or setting a flag, then switches to its service
// task.  The service task performs its processing, then yields to the
// idle task.

// But it wouldn't be overly complicated to add a scheduler, mutexes, and
// condition variables for more elaborate priority-based scheduling.

extern "C" {
uint16_t *task_reg_save _used_;
uint16_t task_leap _used_;
volatile uint16_t task_retval _used_;
uint16_t task_temp _used_;
uint16_t task_temp2 _used_;
uint16_t task_start _used_;
}

class Task {
public:
    typedef void (*StartFunc)();

    static Task _main;   // Main (bootstrap) task
    static Task* _task;  // Current running task

private:
    // Small memory model CPU state for MSP430/430X.  Top 4 bits of registers are
    // effectively unused.
    struct State {
        // Register save slots.  First is PC, then R1 (SP), R2 (SR), no R3 slot, up to R15.
        // R15 is last.  This layout facilitates efficient save and load.
        uint16_t reg[15];
    } _state;

    // Save slots, to explain layout
    enum {
        REG_PC = 0,
        REG_SP,
        REG_SR,
        REG_R4,
        REG_R5,
        REG_R15 = REG_R5 + 10
    };

public:
    Task() { }

    // Prepare to suspend current task as per _task.  This returns 0; non-zero when
    // resumed.  All CPU register state change after this call is "lost".  Interrupts
    // must be disabled.
    static uint16_t prepare_to_suspend() {
        task_retval = 0;
        task_reg_save = _task->_state.reg + 15;  // R15 save slot plus one

        __asm("  mov.w  sp, &task_temp");    // Stash SP
        __asm("  mov.w  &task_reg_save, sp");    // SP now points to R15 save slot + 1
        __asm("  push.w r15");              // Save R15...
        __asm("  push.w r14");
        __asm("  push.w r13");
        __asm("  push.w r12");
        __asm("  push.w r11");
        __asm("  push.w r10");
        __asm("  push.w r9");
        __asm("  push.w r8");
        __asm("  push.w r7");
        __asm("  push.w r6");
        __asm("  push.w r5");
        __asm("  push.w r4");
        __asm("  push.w sr");
        __asm("  push.w &task_temp");        // Stashed SP
        __asm("  mov.w  r12, &task_temp2");
        __asm("  mov.w  sp, r12");
        __asm("  mov.w  &task_temp, sp");
        __asm("  mov.w  pc, -2(r12)");

        // On resume(), execution comes back here with values saved above, except for R12
        // which can be found in task_temp2.
        __asm("  mov.w  &task_temp2, r12");

        return task_retval;
    }

    // Resume current task (as per _task).  Never returns.  Interrupts must be disabled.
#pragma FUNC_NEVER_RETURNS
    static void resume() {
        task_retval = 1;
        task_reg_save = _task->_state.reg+1;   // SP save slot

        __asm("  mov.w  &task_reg_save, r12");
        __asm("  mov.w  @r12+, sp");
        __asm("  push.w -4(r12)");            // Push saved PC for return
        __asm("  nop");                       // Required for SR change
        __asm("  mov.w  @r12+, sr");
        __asm("  nop");                       // Required for SR change
        __asm("  mov.w  @r12+, r4");
        __asm("  mov.w  @r12+, r5");
        __asm("  mov.w  @r12+, r6");
        __asm("  mov.w  @r12+, r7");
        __asm("  mov.w  @r12+, r8");
        __asm("  mov.w  @r12+, r9");
        __asm("  mov.w  @r12+, r10");
        __asm("  mov.w  @r12+, r11");
        __asm("  mov.w  @r12+, r13");        // Skip R12 without affecting SR
        __asm("  mov.w  @r12+, r13");
        __asm("  mov.w  @r12+, r14");
        __asm("  mov.w  @r12+, r15");
        __asm("  mov.w  -8(r12), &task_temp2");
        __asm("  ret");
    }

    // Unguarded task switch.
    //
    // A task switch can be made in an ISR in two ways:
    //  1. Suspend state on entry.  Resume a task on exit.  This is expensive as it incurs
    //     the overhead of saving state for each interrupt.
    //  2. If a switch is needed, then save the state and resume the desired task.  This has
    //     the peculiar effect of suspending a task inside an ISR, and when eventually resumed
    //     again will resume from inside the ISR and return from the interrupt.
    // This function can be used to switch to a specific service task at the end of an ISR.
    static void switch_task(Task& t) {
        // _task can be NULL before we're bootstrapped, yet we may enter an interrupt
        // handler during init.  Just ignore.  Also don't switch to the active task.
        if (_task && &t != _task) {
            if (!prepare_to_suspend()) {
                _task = &t;
                resume();
            }
        }
    }

    // Yield to specific task
    static void yield(Task& t) {
        NoInterruptReent g;
        switch_task(t);
    }

    // Launch task.
    static void launch(Task& t, StartFunc start, void* stack) {
        t._state.reg[REG_SP] = (uint16_t)stack;
        t._state.reg[REG_PC] = (uint16_t)task_wrapper;
        t._state.reg[REG_SR] = __get_SR_register();

        NoInterrupt g;
        task_start = (uint16_t)start;

        switch_task(t);
    }

    // Boostrap: initialize and wrap current execution context in main task
    static void bootstrap() {
        NoInterrupt g;
        _task = &_main;
        //(void)prepare_to_suspend();
        // Fall through, never resuming
    }

private:
#pragma FUNC_NEVER_RETURNS
    static void task_wrapper() {
        const uint16_t start = task_start;
        enable_interrupt();
        ((StartFunc)start)();
        for (;;);
    }
};

#endif // _TASK_H_
