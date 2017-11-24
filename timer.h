#ifndef _TIMER_H_
#define _TIMER_H_

#include <msp430.h>
#include <stdint.h>
#include <limits.h>

#include "config.h"
#include "common.h"

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
    class Future {
        friend class TimerA;
    protected:
        uint32_t _time;

        Future(uint32_t t = 0)  // also default ctor
        : _time(t) {
        }

        Future(const Future& f)
        : _time(f._time) {
        }

    public:
        void adjust(int32_t amount) {
            _time += amount;
        }
    };

    enum {
		MIN_WAIT = 10,
        MAX_WAIT = (1L << 30) - 1, // In CLOCKs
        CLOCK = ::SMCLK/8
    };

    void init() volatile;

    void delay(uint32_t ticks);
    void wait(const Future& future);

    uint32_t ticks() const volatile {
        NoInterrupt g;

        return _time + TA0R;
    }

    Future future(uint32_t ticks) volatile {
        return Future(_timer.ticks() + ticks);
    }

    // Calculate signed distance between current time and future, treated as a
    // sequence with a span of up 2^31 CLOCKs.
    int32_t remainder(const Future& future) const volatile {
        return int32_t(future._time - ticks());
    }

    // True if future is due past due
    bool due(const Future& future) const volatile {
        return remainder(future) <= 0;
    }

private:
    static __interrupt void _ccr0_intr_();
    static __interrupt void _ta_intr_();
};

#endif // _TIMER_H_
