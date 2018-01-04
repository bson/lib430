#ifndef _SYSTIMER_H_
#define _SYSTIMER_H_

#include "common.h"
#include "timer.h"

#define TIMER_USEC(U)  (uint32_t(U)*SYSTIMER_CLOCK/1000000UL)
#define TIMER_MSEC(M)  (uint32_t(M)*SYSTIMER_CLOCK/1000UL)
#define TIMER_SEC(S) (uint32_t(S)*SYSTIMER_CLOCK)

template <typename _BaseTimer>
class SysTimerAB: public _BaseTimer {
public:
    typedef _BaseTimer Timer;

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

        Future& operator=(const Future& f) {
                if (this != &f) {
                    _time = f._time;
                }
                return *this;
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
        MAX_WAIT = (1UL << 30) - 1 // In SYSTIMER_CLOCKs
    };

    void init() {
        Timer::config(Timer::SOURCE_SMCLK, Timer::SOURCE_DIV_8);
        Timer::start(Timer::MODE_CONT);
        Timer::set_counter(0, Timer::ENABLE_INTR, 0xf800);
    }

    void delay(uint32_t ticks);
    void wait(const Future& future);

    uint32_t ticks() const volatile {
        NoInterrupt g;

        return _time + Timer::TA_R;
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
#if 0
    static void _intr_(Timer::VECTOR0) ccr0_intr();
#endif
};

#endif // _SYSTIMER_H_
