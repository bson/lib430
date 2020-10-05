// Copyright (c) 2020 Jan Brittenson
// See LICENSE for details.

#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>
#include "common.h"
#include "systimer.h"
#include "cpu/cpu.h"
#include "config.h"


// Basic task model.

// Most, well actually all, of the code here is inlined to allow the compiler
// to make size-vs-performance tradeoffs.

extern "C" {
uint16_t *task_reg_save _weak_;
uint16_t task_leap _weak_;
volatile uint16_t task_retval _weak_;
uint16_t task_temp _weak_;
uint16_t task_r12 _weak_;
uint16_t task_start _weak_;
}

class Task {
public:
    typedef void (*StartFunc)();

    static Task  _main;  // Main (bootstrap) task, also first in chain
    static Task* _task;  // Current running task

    typedef const void* WChan;  // BSD style wait channel - just an address

private:
    Task* _next;         // Next task in chain

    // Small memory model CPU state for MSP430/MSPX.  Top 4 bits of registers are
    // effectively unused on MSPX.
    struct State {
        // Register save slots.  First is PC, then R2 (SR), no R3 slot, up to R15.
        // SP (R1) is last.  This layout facilitates efficient save and restore.
        uint16_t reg[15];
    } _save;

    // Context register save slots, to explain layout
    enum {
        REG_PC = 0,
        REG_SR,
        REG_R4,
        REG_R5,
        REG_R15 = REG_R5 + 10,
        REG_SP     // SP is saved last
    };

    uint8_t _prio;       // Priority; higher is higher
    volatile uint8_t _state;      // STATE_xxxx

    // When in sleep, this is the wake time, although a task can be
    // woken prematurely through wake().
    SysTimer::Future _sleep;

    WChan _wchan;      // BSD-style wait channel, unused if 0.

public:
    // Task states
    enum {
        STATE_ACTIVE = 0,  // Runnable
        STATE_WAIT,        // Inactive
        STATE_SLEEP        // Inactive, wake at _sleep
    };

    // Task priorities (just a few points)
    enum {
        PRIO_LOW    = 0x10,
        PRIO_MEDIUM = 0x40,
        PRIO_HIGH   = 0xc0
    };

    // Exception.  This is both the catch state (think longjmp) and a
    // 16 bit exception code.
    class Exception {
        Task::State _state;    // State to restore to on exception
        uint16_t _code;        // Exception reason, set when posted,
    public:
        // Predefined codes.  Please conform to these.
        enum {
            CODE_NONE = 0,
            CODE_BERR = 1    // Bus error
        };

        // Catch exceptions.  Returns true on setup, false on a catch.
        // Note that there is no change to the task state.  An exception
        // handler doesn't get used up.  A Task can only have one handler
        // active at any time.  The SR register is reset on reception to
        // the value it had when this function was called.  The task must
        // uninstall the exception before returning from the stack frame
        // it's set in.
        bool receive();

        // Post an exception to current task using RETI.  Useful from
        // a trap or ISR handler, but a task can also use it as a longjmp
        // style context unwind.
        static void post(uint16_t code);

        // Get code
        uint16_t code() const { return _code; }
    };

    Exception* _exception;    // Task exception handler and code, if any

public:
    Task() { }
    ~Task() { }

    // Deactivate and wait to become active.  Takes an optional timer param to set a
    // wait bound.  Primitive function to be used by more convenient wrappers.
    static void wait0(const SysTimer::Future* f, WChan w);

    // Some shorthand forms
    static void force_inline wait(WChan w = 0) { wait0(NULL, w); }
    static void force_inline wait(const SysTimer::Future& f, WChan w = 0) { wait0(&f, w); }
    static void force_inline wait(uint32_t ticks, WChan w = 0) { wait(SysTimer::future(ticks), w); }

    // Activate a task: make it active and switch to it if its priority is higher than
    // the current task.  Must be called with interrupts disabled.
    // The task can be either in a wait or sleep state.
    static void wake(Task& t);

    // Alternate, member form
    void wake() { wake(*this); }

    // Signal a wait channel.  Wakes the highest priority task waiting on it, if any.
    static void signal(WChan w) ;

    // Launch task.
    static void launch(Task& t, StartFunc start, void* stack);

    // Boostrap: initialize and wrap current execution context in main task
    static void bootstrap();

    // Set task priority
    void set_prio(uint8_t prio) { _prio = prio; }

    // Return currently running task
    static Task* self() { return _task; }

protected:
    friend class Exception;
    friend void SysTimer_ccr0_intr();

    // Prepare to suspend current task as per _task.  This returns 0 in setup; non-zero when
    // resumed.  All CPU register state change after this call is "lost" on resume.  Interrupts
    // must be disabled.
    static uint16_t prepare_to_suspend(uint16_t* save_reg);

    // Resume current task (as per _task).  Never returns.  Interrupts must be disabled.
    // Switches via RETI.
#pragma FUNC_NEVER_RETURNS
    static void resume_reti(uint16_t *save_reg);

    // Unguarded task switch.
    //
    // A task switch can be made in an ISR in two ways:
    //  1. Suspend state on entry.  Resume a task on exit.  This is expensive as it incurs
    //     the overhead of saving state for each interrupt.
    //  2. If a switch is needed, then save the state and resume the desired task.  This has
    //     the peculiar effect of suspending a task inside an ISR, and when eventually resumed
    //     agai n will resume from inside the ISR and return from the interrupt.
    // This function can be used to switch to a specific service task at the end of an ISR.
    static void switch_task(Task& t);

    // Pick a task to run.  Returns highest priority active task, otherwise NULL
    // if nothing is active.  Must be called with interrups disabled.
    static Task* pick();

    // Find next due sleeper
    static Task* next_sleeper();

#pragma FUNC_NEVER_RETURNS
    static void task_wrapper();

private:
    Task(const Task&);
    Task& operator=(const Task&);
};

#endif // _TASK_H_
