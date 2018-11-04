// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#include "common.h"
#include "task.h"

Task* Task::_task = NULL;
Task Task::_main;

uint16_t Task::prepare_to_suspend(uint16_t* save_reg) {
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

void Task::wait0(const SysTimer::Future* f, WChan w) {
    {
        NoInterrupt g;

        _task->_wchan = w;
        if (f) {
            _task->_state = STATE_SLEEP;
            _task->_sleep = *f;
            Task* s = next_sleeper();
            if (s) {
                SysTimer::set_sleeper_task(s, s->_sleep);
            } else {
                SysTimer::no_sleeper_task();
            }
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

void Task::wake(Task& t) {
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

void Task::signal(WChan w) {
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

void Task::launch(Task& t, StartFunc start, void* stack) {
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
void Task::bootstrap() {
    NoInterrupt g;
    _task            = &_main;
    _main._next      = NULL;
    _main._prio      = PRIO_LOW;  // Should be changed if desired
    _main._exception = NULL; // No exception context
    _main._wchan     = 0;
}

#pragma FUNC_NEVER_RETURNS
void Task::resume_reti(uint16_t *save_reg) {
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

void Task::switch_task(Task& t) {
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
Task* Task::pick() {
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
Task* Task::next_sleeper() {
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
void Task::task_wrapper() {
    const uint16_t start = task_start;
    enable_interrupt();
    ((StartFunc)start)();
    for (;;)
        LOW_POWER_MODE;
}

bool Task::Exception::receive() {
    NoInterrupt g;
    Task::_task->_exception = this;
    return !Task::prepare_to_suspend(_state.reg);
}

// Post an exception to current task using RETI.  Useful from
// a trap or ISR handler, but a task can also use it as a longjmp
// style context unwind.
void Task::Exception::post(uint16_t code) {
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
