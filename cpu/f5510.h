#ifndef _F5510_H_
#define _F5510_H_

#include "common.h"
#include "timer.h"
#include "usci_a.h"
#include "usci_b.h"
#include "gpio.h"

// Two TimerA's
typedef TimerA3<msp430::TA0CTL, msp430::TA0R, msp430::TA0CCTL0,
                msp430::TA0CCR0, msp430::TA0CCTL1, msp430::TA0CCR1,
                msp430::TA0CCTL2, msp430::TA0CCR2, msp430::TA0IV,
                TIMER0_A0_VECTOR, TIMER0_A1_VECTOR> TimerA3_0;

typedef TimerA3<msp430::TA1CTL, msp430::TA1R, msp430::TA1CCTL0,
                msp430::TA1CCR0, msp430::TA1CCTL1, msp430::TA1CCR1,
                msp430::TA1CCTL2, msp430::TA1CCR2, msp430::TA1IV,
                TIMER1_A0_VECTOR, TIMER1_A1_VECTOR> TimerA3_1;

// System needs a timer someplace.
typedef TimerA3_0 SysTimer;
extern SysTimer _sysTimer;

// UCSI_A1
typedef UCA<msp430::UCA1STAT, msp430::UCA1CTL0, msp430::UCA1CTL1,
            msp430::UCA1MCTL, msp430::UCA1BR0, msp430::UCA1BR1,
            msp430::UCA1RXBUF, msp430::UCA1TXBUF, msp430::UCA1ABCTL,
            msp430::UCA1IRTCTL, msp430::UCA1IRRCTL, msp430::IE2,
            msp430::IFG2, UCA1RXIE, UCA1TXIE, UCA1RXIFG, UCA1TXIFG> UCSI_A1;

// UCSI_B1
typedef UCB<msp430::UCB1STAT, msp430::UCB1CTL0, msp430::UCB1CTL1,
            msp430::UCB1BR0, msp430::UCB1BR1, msp430::UCB1I2CIE,
            msp430::UCB1RXBUF, msp430::UCB1TXBUF, msp430::UCB1I2COA,
            msp430::UCB1I2CSA, msp430::UC0IFG, UCB1TXIFG, UCB1RXIFG> UCSI_B1;

// P1-P6 (not all pins exist externally)
typedef Port<msp430::P1IN, msp430::P1OUT, msp430::P1DIR, msp430::P1SEL,
             msp430::P1SEL2, msp430::P1REN, msp430::P1IFG, msp430::P1IES,
             msp430::P1IE> Port1;

typedef Port<msp430::P2IN, msp430::P2OUT, msp430::P2DIR, msp430::P2SEL,
             msp430::P2SEL2, msp430::P2REN, msp430::P2IFG, msp430::P2IES,
             msp430::P2IE> Port2;

typedef Port<msp430::P3IN, msp430::P3OUT, msp430::P3DIR, msp430::P3SEL,
             msp430::P3SEL3, msp430::P3REN, msp430::P3IFG, msp430::P3IES,
             msp430::P3IE> Port3;

typedef Port<msp430::P4IN, msp430::P4OUT, msp430::P4DIR, msp430::P4SEL,
             msp430::P4SEL4, msp430::P4REN, msp430::P4IFG, msp430::P4IES,
             msp430::P4IE> Port4;

typedef Port<msp430::P5IN, msp430::P5OUT, msp430::P5DIR, msp430::P5SEL,
             msp430::P5SEL5, msp430::P5REN, msp430::P5IFG, msp430::P5IES,
             msp430::P5IE> Port5;

typedef Port<msp430::P6IN, msp430::P6OUT, msp430::P6DIR, msp430::P6SEL,
             msp430::P6SEL6, msp430::P6REN, msp430::P6IFG, msp430::P6IES,
             msp430::P6IE> Port6;

extern volatile uint8_t _dummy_byte;

#endif // _F5510_H_
