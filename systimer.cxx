#ifdef _MAIN_

#include "common.h"
#include "systimer.h"

template <typename T>
void SysTimerAB<T>::delay(uint32_t period)  {
    wait(future(period));
}

template <typename T>
void SysTimerAB<T>::wait(const SysTimerAB<T>::Future& future) {
    while (!due(future))
        ;
}

#if 1
// SysTimer CCR0 interrupt
void _intr_(SysTimer::Timer::VECTOR0)  SysTimer_ccr0_intr() {
    if (SysTimer::Timer::CTL & TAIFG) {
        const uint16_t count = SysTimer::Timer::TA_R;
        SysTimer::Timer::TA_R = 0;
        SysTimer::_time += count;
    }
}
#else
// SysTimer CCR0 interrupt
template <typename T>
void SysTimerAB<T>::ccr0_intr() _used_ {
    if (Timer::CTL & TAIFG) {
        const uint16_t count = Timer::TA_R;
        Timer::TA_R = 0;
        _time += count;
    }
}
#endif

#endif // _MAIN_
