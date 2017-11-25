#include "common.h"
#include "timer.h"

enum {
    INTR_COUNT = 0xf800
};

void TimerA::init() volatile {
    _time = 0;

    // At max CPU frequency SMCLK is 2MHz, and we tick the timer at
    // 250kHz.
    msp430::TA0CTL = TACLR;

     // SMCLK, divide by 8, continuous mode
    msp430::TA0CTL = TASSEL_2|ID_3|MC_2;
    msp430::TA0CCR0  = INTR_COUNT;
    msp430::TA0CCTL0 = CCIE;            // Interrupt on compare
}

// Suppress ULP advisor's suggestion to use a timer instead of an idle loop...
void TimerA::delay(uint32_t period)  {
    _timer.wait(_timer.future(period));
}

#pragma CHECK_ULP("none")
void TimerA::wait(const TimerA::Future& future) {
    while (!_timer.due(future))
        ;
}
#pragma RESET_ULP("all")

// CCR0 interrupt
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TimerA::_ccr0_intr_() {
    if (msp430::TA0CTL & TAIFG) {
        const uint16_t count = msp430::TA0R;
        msp430::TA0R = 0;
        _timer._time += count;
    }
}

// TA interrupt
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TimerA::_ta_intr_() {
}
