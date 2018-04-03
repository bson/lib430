// Copyright (c) 2018 Jan Brittenson
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

    typedef const void *WChan;  // BSD style wait channel - just an address

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
    uint8_t _state;      // STATE_xxxx

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
        bool receive() {
            NoInterrupt g;
            Task::_task->_exception = this;
            return !Task::prepare_to_suspend(_state.reg);
        }

        // Post an exception to current task using RETI.  Useful from
        // a trap or ISR handler, but a task can also use it as a longjmp
        // style context unwind.
        static void post(uint16_t code) {
            NoInterrupt g;
            if (!Task::_task) {
                // Not bootstrapped yet
                return;
            }

            if (Task::_task->_exception) {
                Exception* e = Task::_task->_exception;
                e->_code = code;
                resume_reti(e->_state.reg);
            }
        }

        // Get code
        uint16_t code() const { return _code; }
    };

    Exception* _exception;    // Task exception handler and code, if any

public:
    Task() { }
    ~Task() { }

    // Deactivate and wait to become active.  Takes an optional timer param to set a
    // wait bound.  Primitive function to be used by more convenient wrappers.
    static void wait0(const SysTimer::Future* f, WChan w) {
        {
            NoInterrupt g;

            _task->_wchan = w;
            if (f) {
                _task->_state = STATE_SLEEP;
                _task->_sleep = *f;
                Task* s = next_sleeper();
                SysTimer::set_sleeper_task(s, s->_sleep);
            } else {
                _task->_state = STATE_WAIT;
            }

            Task *t = pick();  // Pick another task
            if (t)
                switch_task(*t);
        }

        while (_task->_state != STATE_ACTIVE)
            LOW_POWER_MODE;
    }

    // Some shorthand forms
    static void wait(WChan w = 0) { wait0(NULL, w); }
    static void wait(const SysTimer::Future& f, WChan w = 0) { wait0(&f, w); }
    static void wait(uint32_t ticks, WChan w = 0) { wait(SysTimer::future(ticks), w); }

    // Activate a task: make it active and switch to it if its priority is higher than
    // the current task.  Must be called with interrupts disabled.
    // The task can be either in a wait or sleep state.
    static void wake(Task& t) {
        switch (t._state) {
        case STATE_SLEEP: {
            if (SysTimer::sleeper() == &t) {
                // Waking the active sleeper... set up the next one.
                t._state = STATE_ACTIVE;
                Task* s = next_sleeper();
                if (s) {
                    SysTimer::set_sleeper_task(s, s->_sleep);
                } else {
                    SysTimer::no_sleeper_task();
                }
            }
        }
        // FALLTHRU
        case STATE_WAIT:
            t._state = STATE_ACTIVE;
            t._wchan = 0;
            if (_task && (t._prio > _task->_prio || _task->_state != STATE_ACTIVE))
                switch_task(t);
            break;
        default:
            break;
        }
    }

    // Signal a wait channel.  Wakes the highest priority task waiting on it, if any.
    static void signal(WChan w) {
        Task* best = NULL;
        for (Task* t = &_main; t; t = t->_next) {
            if ((t->_state == STATE_SLEEP || t->_state == STATE_WAIT) && t->_wchan == w &&
                    (!best || t->_prio > best->_prio)) {
                best = t;
            }
        }
        if (best)
            wake(*best);
    }


    // Launch task.
    static void launch(Task& t, StartFunc start, void* stack) {
        t._save.reg[REG_SP] = (uint16_t)stack;
        t._save.reg[REG_PC] = (uint16_t)task_wrapper;
        t._save.reg[REG_SR] = __get_SR_register();
        t._state            = STATE_ACTIVE;
        t._exception        = NULL;
        t._wchan            = 0;

        NoInterrupt g;
        task_start = (uint16_t)start;

        t._next = _task->_next;
        _task->_next = &t;

        switch_task(t);
    }

    // Boostrap: initialize and wrap current execution context in main task
    static void bootstrap() {
        NoInterrupt g;
        _task            = &_main;
        _main._next      = NULL;
        _main._prio      = PRIO_LOW;  // Should be changed if desired
        _main._exception = NULL; // No exception context
        _main._wchan     = 0;
    }

    // Set task priority
    void set_prio(uint8_t prio) { _prio = prio; }

    // Return self
    static Task* self() { return _task; }

protected:
    friend class Exception;
    friend void SysTimer_ccr0_intr();

    // Prepare to suspend current task as per _task.  This returns 0 in setuo; non-zero when
    // resumed.  All CPU register state change after this call is "lost" on resume.  Interrupts
    // must be disabled.
    static uint16_t prepare_to_suspend(uint16_t* save_reg) {
        task_retval = 0;
        task_reg_save = save_reg + REG_SP + 1;  // SP save slot plus one

        // A bit cumbersome; would be neater with gcc style asm, but the TI compiler doesn't
        // support it.
        __asm("  mov.w  sp, &task_temp");    // Stash SP
        __asm("  mov.w  &task_reg_save, sp");// SP now points to SP save slot + 1
        __asm("  push.w &task_temp");        // Save stashed SP
        __asm("  pushm.w #12, r15");         // Save R15-R4
        __asm("  push.w sr");                // Save SR
        __asm("  mov.w  r12, &task_r12");
        __asm("  mov.w  sp, r12");
        __asm("  mov.w  &task_temp, sp");    // Restore stashed SP
        __asm("  mov.w  pc, -2(r12)");       // Save PC

        // On resume(), execution comes back here with values saved above, except for R12
        // which can be found in the global task_r12.
        __asm("  mov.w  &task_r12, r12");

        return task_retval;
    }

    // Resume current task (as per _task).  Never returns.  Interrupts must be disabled.
#pragma FUNC_NEVER_RETURNS
    static void resume(uint16_t *save_reg) {
        task_retval = 1;
        task_reg_save = save_reg;        // PC save slot

        __asm("  mov.w  &task_reg_save, sp");
        __asm("  pop.w  &task_leap");    // Saved PC
        __asm("  nop");                  // Drain pipeline SR dependency
        __asm("  pop.w  sr");            // Saved SR
        __asm("  nop");                  // Drain pipeline SR dependency
        __asm("  popm.w #12, r15");      // Restore R4-15
        __asm("  mov.w r12, &task_r12"); // Set up R12 for resume
        __asm("  pop.w sp");             // Restore SP (also avoids errata CPU46)
        __asm("  mov.w &task_leap, pc");
    }

    // Like resume(), except returns via RETI
#pragma FUNC_NEVER_RETURNS
    static void resume_reti(uint16_t *save_reg) {
        task_retval = 1;
        task_reg_save = save_reg;         // PC save slot

        __asm("  mov.w  &task_reg_save, sp");
        __asm("  pop.w  &task_leap");     // Saved PC
        __asm("  nop");                   // Drain pipeline SR dependency
        __asm("  pop.w  sr");             // Saved SR
        __asm("  nop");                   // Drain pipeline SR dependency
        __asm("  popm.w #12, r15");       // Restore R4-15
        __asm("  mov.w  r12, &task_r12"); // Set up R12 for resume
        __asm("  pop.w  sp");             // Restore SP (also avoids errata CPU46)
        __asm("  push.w &task_leap");
        __asm("  push.w sr");
        __asm("  reti");
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
        // handler during init.  Just ignore.  Also don't switch to the task already on CPU.
        if (_task && &t != _task) {
            if (!prepare_to_suspend(_task->_save.reg)) {
                _task = &t;
                // Currently always resume via RETI.  Harmless when not in an ISR, but
                // there may hidden logic that depends on it when resuming a different
                // task from an ISR.
                resume_reti(_task->_save.reg);
            }
        }
    }

    // Pick a task to run.  Returns highest priority active task, otherwise NULL
    // if nothing is active.  Must be called with interrups disabled.
    static Task* pick() {
        Task* best = NULL;
        uint8_t best_prio = 0;
        for (Task* t = &_main; t; t = t->_next) {
            if (t->_state == STATE_ACTIVE) {
                if (!best || t->_prio > best_prio) {
                    best = t;
                    best_prio = t->_prio;
                }
            }
        }
        return best;
    }

    // Find next due sleeper
    static Task* next_sleeper() {
        Task* next = NULL;
        SysTimer::Future next_due;
        for (Task* t = &_main; t; t = t->_next) {
            if (t->_state == STATE_SLEEP) {
                if (!next || t->_sleep < next_due) {
                    next = t;
                    next_due = t->_sleep;
                }
            }
        }
        return next;
    }

#pragma FUNC_NEVER_RETURNS
    static void task_wrapper() {
        const uint16_t start = task_start;
        enable_interrupt();
        ((StartFunc)start)();
        for (;;)
            LOW_POWER_MODE;
    }

private:
    Task(const Task&);
    Task& operator=(const Task&);
};

#endif // _TASK_H_
