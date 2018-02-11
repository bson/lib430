#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>
#include "common.h"
#include "systimer.h"
#include "cpu/cpu.h"


// Simple task abstraction.  Its main purpose is to support a simple
// model where there is a single main or "idle" context, and additional
// service tasks for e.g. USB or I2C Device support to do processing too
// complex for an ISR.  In this case the ISR notes work to be done, e.g.
// by registering an event or setting a flag, then switches to its service
// task.  The service task performs its processing, then yields to the
// idle task.

// Most, well actually all, of the code here is inlined to allow the compiler
// to make size-vs-performance tradeoffs (inlining).

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

private:
    Task* _next;         // Next task in chain

    // Small memory model CPU state for MSP430/430X.  Top 4 bits of registers are
    // effectively unused.
    struct State {
        // Register save slots.  First is PC, then R1 (SP), R2 (SR), no R3 slot, up to R15.
        // R15 is last.  This layout facilitates efficient save and load.
        uint16_t reg[15];
    } _save;

    // Save slots, to explain layout
    enum {
        REG_PC = 0,
        REG_SP,
        REG_SR,
        REG_R4,
        REG_R5,
        REG_R15 = REG_R5 + 10
    };

    uint8_t _prio;       // Priority; higher is higher
    uint8_t _state;

    SysTimer::Future _sleep;

public:
    // Task states
    enum {
        STATE_ACTIVE = 0,  // Runnable
        STATE_WAIT,        // Inactive
        STATE_SLEEP        // Sleeping into _sleep
    };

    // Task priorities (just a few points)
    enum {
        PRIO_LOW    = 0x10,
        PRIO_MEDIUM = 0x40,
        PRIO_HIGH   = 0xc0
    };

    // Exception.  This is both the catch state (tink longjmp) and a
    // 16 bit exception code.
    class Exception {
        Task::State _state;    // State to restore to on exception
        uint16_t _code;
    public:
        // Predefined codes.  Please conform to these.
        enum {
            CODE_NONE = 0,
            CODE_BERR = 1    // Bus error
        };

        // Catch exceptions.  Returns true on setup, false on a catch.
        // Note that there is no change to the task state.  An exception
        // handler doesn't get used up.  A Task can only have one handler
        // active at any time.
        bool receive() {
            NoInterruptReent g;
            Task::_task->_exception = this;
            return (!Task::prepare_to_suspend(_state.reg));
        }

        // Post an exception to current task
        static void post(uint16_t code) {
            NoInterruptReent g;
            if (!Task::_task) {
                // No task bootstrapped
                return;
            }

            if (Task::_task->_exception) {
                Exception* e = Task::_task->_exception;
                e->_code = code;
                resume(e->_state.reg);
            }
        }

        // Get code
        uint16_t code() const { return _code; }
    };

    Exception* _exception;    // Task exception handler and code, if any

public:
    Task() { }
    ~Task() { }

    // Yield to specific task, ignoring priority.  This permits a task to hand over
    // temporarily to a lower priority task.  The lower priority one will run until
    // it yields, activates a higher priority task, or enters wait.
    static void yield(Task& t) {
        NoInterruptReent g;
        if (t._state != STATE_SLEEP)  {
            t._state = STATE_ACTIVE;
            switch_task(t);
        }
    }

    // Activate a task: make it active and switch to it if its priority is higher than
    // the current task.  Must be called with interrupts disabled.
    static void activate(Task& t) {
        if (t._state == STATE_WAIT) {
            t._state = STATE_ACTIVE;

            if (_task && (t._prio > _task->_prio || _task->_state != STATE_ACTIVE))
                switch_task(t);
        }
    }

    // Deactivate and wait to become active
    static void wait() {
        {
            NoInterruptReent g;
            _task->_state = STATE_WAIT;
            Task *t = pick();  // Pick another task
            if (t)
                switch_task(*t);
        }
        while (_task->_state != STATE_ACTIVE)
            LPM3;
    }

    // Wake sleeping task. Must be called with interrupts disabled. Callable from an ISR.
    static void wake(Task& t) {
        if (t._state == STATE_SLEEP) {
            t._state = STATE_ACTIVE;

            Task* s = next_sleeper();
            if (s) {
                SysTimer::set_sleeper_task(s, s->_sleep);
            }
            if (t._prio > _task->_prio || _task->_state != STATE_ACTIVE) {
                switch_task(t);
            }
        }
    }

    // Task sleep.
    static void sleep(const SysTimer::Future& f) {
        {
            NoInterruptReent g;
            _task->_state = STATE_SLEEP;
            _task->_sleep = f;
            Task* s = next_sleeper();
            SysTimer::set_sleeper_task(s, s->_sleep);
            Task *t = pick();  // Pick another task
            if (t)
                switch_task(*t);
        }
        while (_task->_state != STATE_ACTIVE)
            LPM3;
    }

    // Short hand to sleep in ticks
    static void sleep(int32_t ticks) { sleep(SysTimer::future(ticks)); }

    // Launch task.
    static void launch(Task& t, StartFunc start, void* stack) {
        t._save.reg[REG_SP] = (uint16_t)stack;
        t._save.reg[REG_PC] = (uint16_t)task_wrapper;
        t._save.reg[REG_SR] = __get_SR_register();
        t._state            = STATE_ACTIVE;
        t._exception        = NULL;

        NoInterrupt g;
        task_start = (uint16_t)start;

        t._next = _task->_next;
        _task->_next = &t;

        switch_task(t);
    }

    // Boostrap: initialize and wrap current execution context in main task
    static void bootstrap() {
        NoInterrupt g;
        _task = &_main;
        _main._next = NULL;
        _main._prio = PRIO_LOW;  // Should be changed if desired
        _main._exception = NULL; // No exception context
    }

    // Set task priority
    void set_prio(uint8_t prio) { _prio = prio; }

    // Return self
    static Task* self() { return _task; }

protected:
    friend class Exception;

    // Prepare to suspend current task as per _task.  This returns 0; non-zero when
    // resumed.  All CPU register state change after this call is "lost".  Interrupts
    // must be disabled.
    static uint16_t prepare_to_suspend(uint16_t* save_reg) {
        task_retval = 0;
        task_reg_save = save_reg + 15;  // R15 save slot plus one

        // MSP430 can't have autoinc/autodec in dst operand, so use SP and push.

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
        __asm("  push.w sr");                // ...through R2
        __asm("  push.w &task_temp");        // Save stashed SP
        __asm("  mov.w  r12, &task_r12");
        __asm("  mov.w  sp, r12");
        __asm("  mov.w  @sp, sp");           // Restore SP
        __asm("  mov.w  pc, -2(r12)");

        // On resume(), execution comes back here with values saved above, except for R12
        // which can be found in the global task_r12.
        __asm("  mov.w  &task_r12, r12");

        return task_retval;
    }

    // Resume current task (as per _task).  Never returns.  Interrupts must be disabled.
#pragma FUNC_NEVER_RETURNS
    static void resume(uint16_t *save_reg) {
        task_retval = 1;
        task_reg_save = save_reg+1;   // SP save slot

        __asm("  mov.w  &task_reg_save, r12");
        __asm("  mov.w  @r12+, sp");
        __asm("  mov.w  -4(r12), &task_leap"); // Set up saved PC
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
        __asm("  mov.w  @r12+, &task_r12");
        __asm("  mov.w  @r12+, r13");
        __asm("  mov.w  @r12+, r14");
        __asm("  mov.w  @r12+, r15");
        __asm("  mov.w  &task_leap, pc");   // Weeee....
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
                resume(_task->_save.reg);
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
            LPM3;
    }

private:
    Task(const Task&);
    Task& operator=(const Task&);
};

#endif // _TASK_H_
