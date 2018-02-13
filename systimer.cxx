#ifdef _MAIN_

#include "common.h"
#include "systimer.h"
#include "task.h"
#include "config.h"

volatile uint32_t SysTimer::_time;
volatile void* SysTimer::_sleeper;
volatile SysTimer::Future SysTimer::_sleep;

// SysTimer CCR0 interrupt
void _intr_(SysTimer::Timer::VECTOR0)  SysTimer_ccr0_intr() {
    Task* task_wake = NULL;

    if (SysTimer::Timer::CTL & TAIFG) {
        const uint16_t count = SysTimer::Timer::TA_R;
        SysTimer::Timer::TA_R = 0;
        SysTimer::_time += count;

        if (SysTimer::_sleeper && SysTimer::due((SysTimer::Future&)SysTimer::_sleep)) {
            task_wake = (Task*)SysTimer::_sleeper;
        } else {
            SysTimer::update_ccr();
        }
    }

    // This can cause a task switch so needs to happen last
    if (task_wake) {
        Task::wake(*task_wake);
        LOW_POWER_MODE_EXIT;
    }
}

#endif // _MAIN_
