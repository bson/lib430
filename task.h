#ifndef _TASK_H_
#define _TASK_H_

extern "C" {
uint16_t *reg_save _used_;
uint16_t leap _used_;
volatile uint16_t retval _used_;
uint16_t tasktemp _used_;
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
        retval = 0;
        reg_save = _task->_state.reg + 14;          // R15 save slot

        __asm("  mov.w  sp, &tasktemp");       // Stash SP
        __asm("  mov.w  &reg_save, sp");      // SP now points to R15 save slot
        __asm("  push.w r15");               // Save R15...
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
        __asm("  push.w &tasktemp");     // Stashed SP
        __asm("  mov.w  sp, r12");
        __asm("  mov.w  &tasktemp, sp");
        __asm("  mov.w  pc, -2(r12)");

        // On resume(), execution comes back here with values saved above.

        return retval;
    }

    // Resume current task (as per _task).  Never returns.  Interrupts must be disabled.
#pragma FUNC_NEVER_RETURNS
    static void resume() {
        retval = 1;
        reg_save = _task->_state.reg;        // PC save slot

        __asm("  mov.w  &reg_save, r12");
        __asm("  mov.w  @r12+, &leap");       // Saved PC
        __asm("  mov.w  @r12+, sp");
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
        __asm("  mov.w  @r12+, r12");        // Pro forma
        __asm("  mov.w  @r12+, r13");
        __asm("  mov.w  @r12+, r14");
        __asm("  mov.w  @r12+, r15");
        __asm("  mov.w  &leap, pc");
    }

    // A task switch can be made in an ISR in two ways:
    //  1. Suspend state on entry.  Resume a task on exit.  This is expensive as it incurs
    //     the overhead of saving state for each interrupt.
    //  2. If a switch is needed, then save the state and resume the desired task.  This has
    //     the peculiar effect of suspending a task inside an ISR, and when eventually resumed
    //     again will resume from inside the ISR and return from the interrupt though outside
    //     an interrupt context.
    static void switch_task(Task* t) {
        if (_task && t && t != _task) {
            if (!prepare_to_suspend()) {
                _task = t;
                resume();
            }
        }
    }

    // Yield to specific task
    static void yield(Task* t) {
        NoInterruptReent g;
        switch_task(t);
    }

    // Launch task.
    static void launch(Task *t, StartFunc start, void* stack) {
        t->_state.reg[REG_SP] = (uint16_t)stack;
        t->_state.reg[REG_PC] = (uint16_t)task_wrapper;
        task_start = (uint16_t)start;

        yield(t);
    }

    // Boostrap: initialize and wrap current execution context in main task
    static void bootstrap() {
        NoInterrupt g;
        _task = &_main;
        (void)prepare_to_suspend();
    }

private:
    static void task_wrapper() {
        const uint16_t start = task_start;
        enable_interrupt();
        ((StartFunc)start)();
        for (;;);
    }
};

#endif // _TASK_H_
