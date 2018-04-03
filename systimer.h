// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

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

    class Future {
        uint32_t _time;
    public:
        explicit Future(uint32_t t = 0)  // also default ctor
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

        volatile Future& operator=(const Future& f) volatile {
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
        uint32_t time() const volatile { return _time; }
    };
protected:
    friend void SysTimer_ccr0_intr();

    volatile static uint32_t _time;
    volatile static Future _sleep;    // If a task is sleeping, this is its wake time
    volatile static void* _sleeper;   // If a task is sleeping, this points to it
public:

    enum {
        MIN_WAIT = 10,
        MAX_WAIT = (1UL << 30) - 1, // In SYSTIMER_CLOCKs
        CCR       = 0,              // CCR to use (not that CCR0 has its own vector)
        CCR_LIMIT = 0xf800         // Timer interrupt point
    };

    static void init() {
        Timer::config(Timer::SOURCE_ACLK, Timer::SOURCE_DIV_8);
        Timer::start(Timer::MODE_CONT);
        Timer::set_counter(CCR, Timer::ENABLE_INTR, CCR_LIMIT);
    }

    static void delay(uint32_t ticks) {
        wait(future(ticks));
    }

    static void wait(const Future& future) {
        while (!due(future))
            ;
    }

    static uint32_t ticks() {
        NoInterrupt g;

        return _time + Timer::TA_R;
    }

    static Future future(uint32_t t) {
        return Future(ticks() + t);
    }

    // Calculate signed distance between current time and future, treated as a
    // sequence with a span of up 2^31 CLOCKs.
    static int32_t remainder(const Future& future) {
        return int32_t(future.time() - ticks());
    }

    // True if future is due past due
    static bool due(const Future& future) {
        return remainder(future) <= 0;
    }

    // Set sleeper task.  Call with interrupts disabled.
    static void set_sleeper_task(void* task, const Future& sleep) {
        if (task != _sleeper) {
            _sleeper = task;
            _sleep = (Future&)sleep;
            update_ccr();
        }
    }

    // No sleeper task.  Call with interrupts disabled.
    static void no_sleeper_task() {
        _sleeper = NULL;
        update_ccr();
    }

    // Get current sleeper task.
    static void* sleeper() { return (void*)_sleeper; }

    // Update CCR.  Must be called with interrupts disabled.
    static void update_ccr() {
        if (!_sleeper) {
            Timer::set_counter(CCR, Timer::ENABLE_INTR, CCR_LIMIT);
            return;
        }

        const int32_t r = _sleep.time() - _time;
        if (r >= CCR_LIMIT) {
            Timer::set_counter(CCR, Timer::ENABLE_INTR, CCR_LIMIT);
        } else {
            const uint16_t r2 = r;
            // If it's so close it's practically due, set count slightly ahead of
            // the timer register and let the ISR run immediately.
            // XXX Set the IFG here instead?
            Timer::set_counter(CCR, Timer::ENABLE_INTR, max<uint16_t>(Timer::TA_R + 8, r2));
        }
    }
};

#endif // _SYSTIMER_H_
