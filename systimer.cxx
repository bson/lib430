#ifdef _MAIN_

#include "common.h"
#include "systimer.h"

// SysTimer CCR0 interrupt
void _intr_(SysTimer::Timer::VECTOR0)  SysTimer_ccr0_intr() {
    if (SysTimer::Timer::CTL & TAIFG) {
        const uint16_t count = SysTimer::Timer::TA_R;
        SysTimer::Timer::TA_R = 0;
        SysTimer::_time += count;
    }
}

#endif // _MAIN_
