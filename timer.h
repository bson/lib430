#ifndef _TIMER_H_
#define _TIMER_H_

#include "common.h"
#include "config.h"
#include "accessors.h"

#define TIMER_USEC(U)  (uint32_t(U)*SysTimer::CLOCK/1000000UL)
#define TIMER_MSEC(M)  (uint32_t(M)*SysTimer::CLOCK/1000UL)
#define TIMER_SEC(S) (uint32_t(S)*SysTimer::CLOCK)


// TIMER_A3
// This basic timer runs in continuous UP mode, rolling over.
// Compare registers are used relative to the value.
template <volatile uint16_t& _CTL,
          volatile uint16_t& _R,
          volatile uint16_t& _CCTL0,
          volatile uint16_t& _CCR0,
          volatile uint16_t& _CCTL1,
          volatile uint16_t& _CCR1,
          volatile uint16_t& _CCTL2,
          volatile uint16_t& _CCR2,
          volatile uint16_t& _IV,
          uint INTVEC0,
          uint INTVEC1>
class TimerA3 {

public:
    volatile static uint32_t _time;

    class Future {
        uint32_t _time;
    public:
        Future(uint32_t t = 0)  // also default ctor
        : _time(t) {
        }

        Future(const Future& f)
        : _time(f._time) {
        }

        bool operator<(const Future& rhs) const {
            return int32_t(_time - rhs._time) < 0;
        }

        void adjust(int32_t amount) {
            _time += amount;
        }

        uint32_t time() const { return _time; }
    };

    enum {
		MIN_WAIT = 10,
        MAX_WAIT = (1UL << 30) - 1, // In CLOCKs
        CLOCK = ::SMCLK/8,
        INTR_COUNT = 0xf800
    };

    ACCESSOR(volatile uint16_t&, getCTL, _CTL);
    ACCESSOR(volatile uint16_t&, getR, _R);
    ACCESSOR(volatile uint16_t&, getCCTL0, _CCTL0);
    ACCESSOR(volatile uint16_t&, getCCR0, _CCR0);
    ACCESSOR(volatile uint16_t&, getCCTL1, _CCTL1);
    ACCESSOR(volatile uint16_t&, getCCR1, _CCR1);
    ACCESSOR(volatile uint16_t&, getCCTL2, _CCTL2);
    ACCESSOR(volatile uint16_t&, getCCR2, _CCR2);
    ACCESSOR(volatile uint16_t&, getIV, _IV);

    void init() volatile;

    void delay(uint32_t ticks);
    void wait(const Future& future);

    uint32_t ticks() const volatile {
        NoInterrupt g;

        return _time + TA_R;
    }

    Future future(uint32_t t) volatile {
        return Future(ticks() + t);
    }

    // Calculate signed distance between current time and future, treated as a
    // sequence with a span of up 2^31 CLOCKs.
    int32_t remainder(const Future& future) const volatile {
        return int32_t(future.time() - ticks());
    }

    // True if future is due past due
    bool due(const Future& future) const volatile {
        return remainder(future) <= 0;
    }
};

#endif // _TIMER_H_
