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
