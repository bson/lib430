#ifndef _TIMER_H_
#define _TIMER_H_

#include <msp430.h>
#include <stdint.h>

#include "config.h"

#define TIMER_USEC(U)  (uint32_t(U)*TimerA::CLOCK/1000000UL)
#define TIMER_MSEC(M)  (uint32_t(M)*TimerA::CLOCK/1000UL)
#define TIMER_SEC(S) (uint32_t(S)*TimerA::CLOCK)


// System needs a timer someplace.
extern class TimerA _timer;

// TIMER_A
// This basic timer runs in continuous UP mode, rolling over.
// Compare registers are used relative to the value.
class TimerA {
    volatile uint32_t _time;

public:
    enum {
		MIN_WAIT = 10,
        MAX_WAIT = uint32_t(-1),
        CLOCK = ::SMCLK/8
    };

    void init() volatile;

    void delay(uint32_t period) volatile;
    uint32_t ticks() volatile { return _time + TA0R; }
    void remove(uint32_t amount) volatile { _time -= amount; }

private:
    static __interrupt void _ccr0_intr_();
    static __interrupt void _ta_intr_();
};

#endif // _TIMER_H_
