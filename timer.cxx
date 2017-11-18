#include <msp430.h>
#include <stdint.h>

#include "timer.h"

enum {
    INTR_COUNT = 0xf800
};

void TimerA::init() volatile {
    _time = 0;

    // At max CPU frequency SMCLK is 2MHz, and we tick the timer at
    // 250kHz.
    TA0CTL = TACLR;
    TA0CTL = TASSEL_2|ID_3|MC_2; // SMCLK, divide by 8, continuous mode
    TA0CCR0  = INTR_COUNT;
    TA0CCTL0 = CCIE;            // Interrupt on compare
}

// Suppress ULP advisor's suggestion to use a timer instead of an idle loop...
#pragma CHECK_ULP("none")
void TimerA::delay(uint32_t period) volatile {
    const uint32_t end = ticks() + period;
    while (ticks() < end)  // ...here
        ;
}
#pragma RESET_ULP("all")

// CCR0 interrupt
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TimerA::_ccr0_intr_() {
    if (TA0CTL & TAIFG) {
        const uint16_t count = TA0R;
        TA0R -= INTR_COUNT + 4;
        _timer._time += count + 4;
    }
}

// TA interrupt
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TimerA::_ta_intr_() {
}
