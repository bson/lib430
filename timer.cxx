#include "common.h"
#include "timer.h"

enum {
    INTR_COUNT = 0xf800
};

#define TEMPLATE_TIMER_A3 \
template <volatile uint16_t& _CTL,   \
          volatile uint16_t& _R,     \
          volatile uint16_t& _CCTL0, \
          volatile uint16_t& _CCR0,  \
          volatile uint16_t& _CCTL1, \
          volatile uint16_t& _CCR1,  \
          volatile uint16_t& _CCTL2, \
          volatile uint16_t& _CCR2,  \
          volatile uint16_t& _IV>

#define PARMS _CTL,_R,_CCTL0,_CCR0,_CCTL1,_CCR1,_CCTL2,_CCR2,_IV

TEMPLATE_TIMER_A3
void TimerA3<PARMS>::init() volatile {
    _time = 0;

    // At max CPU frequency SMCLK is 2MHz, and we tick the timer at
    // 250kHz.
    CTL = TACLR;

     // SMCLK, divide by 8, continuous mode
    CTL = TASSEL_2|ID_3|MC_2;
    TA_CCR0  = INTR_COUNT;
    TA_CCTL0 = CCIE;            // Interrupt on compare
}

// Suppress ULP advisor's suggestion to use a timer instead of an idle loop...
TEMPLATE_TIMER_A3
void TimerA3<PARMS>::delay(uint32_t period)  {
    wait(future(period));
}

#pragma CHECK_ULP("none")
TEMPLATE_TIMER_A3
void TimerA3<PARMS>::wait(const TimerA3<PARMS>::Future& future) {
    while (!due(future))
        ;
}
#pragma RESET_ULP("all")

// CCR0 interrupt
#pragma vector=TIMER0_A0_VECTOR
TEMPLATE_TIMER_A3
__interrupt void TimerA3<PARMS>::_ccr0_intr_() {
    if (_sysTimer.CTL & TAIFG) {
        const uint16_t count = _sysTimer.TA_R;
        _sysTimer.TA_R = 0;
        _sysTimer._time += count;
    }
}

// TA interrupt
#pragma vector=TIMER0_A1_VECTOR
TEMPLATE_TIMER_A3
__interrupt void TimerA3<PARMS>::_ta_intr_() {
}
