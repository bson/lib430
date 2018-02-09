#ifndef _TIMER_H_
#define _TIMER_H_

#include "common.h"
#include "config.h"
#include "accessors.h"


// TIMER_A3, TIMER_A5, TIMER_B
template <volatile uint16_t& _CTL,
          volatile uint16_t& _R,
          volatile uint16_t& _CCTL0,
          volatile uint16_t& _CCR0,
          volatile uint16_t& _CCTL1,
          volatile uint16_t& _CCR1,
          volatile uint16_t& _CCTL2,
          volatile uint16_t& _CCR2,
          volatile uint16_t& _IV,
          uintptr_t INTVEC0,
          uintptr_t INTVEC1>
class TimerA3 {

public:
    enum {
        VECTOR0 = INTVEC0,
        VECTOR1 = INTVEC1
    };

    // config() parameters
    enum {
        SOURCE_SMCLK = TASSEL_2,
        SOURCE_ACLK  = TASSEL_1,
        SOURCE_EXTCLK= TASSEL_3,
        SOURCE_DIV_1 = ID_0,
        SOURCE_DIV_2 = ID_1,
        SOURCE_DIV_4 = ID_2,
        SOURCE_DIV_8 = ID_3
    };

    // start() parameters
    enum {
        MODE_MASK    = MC_3,
        MODE_STOP    = MC_0,
        MODE_UP      = MC_1, // Count up to CCR0
        MODE_CONT    = MC_2, // Count up of ~0 and roll over
        MODE_SEESAW  = MC_3  // Count up to CCR0, then down to 0
    };

    // set_counter() ctl parameters
    enum {
        ENABLE_INTR  = CCIE
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

    static void config(uint16_t source, uint16_t divider) {
        CTL  = source|divider|MODE_STOP;
    }

    static void start(uint16_t mode, uint16_t count = 0) {
        CTL      = (CTL & ~MODE_MASK) | mode;
        TA_R     = 0;
    }

    static void stop() {
        CTL = (CTL & ~MODE_MASK) | MODE_STOP;
    }

    static void set_counter(uint8_t num, uint16_t ctl, uint16_t val) {
        volatile uint16_t *timer_ctl = &TA_CCTL0;
        volatile uint16_t *timer_ccr = &TA_CCR0;
        timer_ccr[num] = val;
        timer_ctl[num] = ctl;
    }

    static void set_ccr(uint8_t num, uint16_t val) {
        volatile uint16_t *timer_ccr = &TA_CCR0;
        timer_ccr[num] = val;
    }
};

#endif // _TIMER_H_
