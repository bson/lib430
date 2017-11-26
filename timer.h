#ifndef _TIMER_H_
#define _TIMER_H_

#include "common.h"
#include "config.h"
#include "accessors.h"

#define TIMER_USEC(U)  (uint32_t(U)*TimerA::CLOCK/1000000UL)
#define TIMER_MSEC(M)  (uint32_t(M)*TimerA::CLOCK/1000UL)
#define TIMER_SEC(S) (uint32_t(S)*TimerA::CLOCK)


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
    static _intr_(INTVEC0) void _ccr0_intr_();
    static _intr_(INTVEC1) void _ta_intr_();
};

#endif // _TIMER_H_
