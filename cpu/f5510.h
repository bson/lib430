// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef _F5510_H_
#define _F5510_H_

#include "common.h"
#include "timer.h"
#include "systimer.h"
#include "usci_a.h"
#include "usci_b.h"
#include "gpio.h"

// 3x TimerA, 1x TimerB
// A and B differ in the number of CCR registers; TimerB has 8, plus an expansion register.
typedef TimerA3<TA0CTL, TA0R, TA0CCTL0,
                TA0CCR0, TA0CCTL1, TA0CCR1,
                TA0CCTL2, TA0CCR2, TA0IV,
                TIMER0_A0_VECTOR, TIMER0_A1_VECTOR> TimerA3_0;

typedef TimerA3<TA1CTL, TA1R, TA1CCTL0,
                TA1CCR0, TA1CCTL1, TA1CCR1,
                TA1CCTL2, TA1CCR2, TA1IV,
                TIMER1_A0_VECTOR, TIMER1_A1_VECTOR> TimerA3_1;

typedef TimerA3<TA2CTL, TA2R, TA2CCTL0,
                TA2CCR0, TA2CCTL1, TA2CCR1,
                TA2CCTL2, TA2CCR2, TA2IV,
                TIMER2_A0_VECTOR, TIMER2_A1_VECTOR> TimerA3_2;

typedef TimerA3<TB0CTL, TB0R, TB0CCTL0,
                TB0CCR0, TB0CCTL1, TB0CCR1,
                TB0CCTL2, TB0CCR2, TB0IV,
                TIMER0_B0_VECTOR, TIMER0_B1_VECTOR> TimerB_0;

// System needs a timer someplace.
typedef SysTimerAB<TimerA3_0> SysTimer;

extern volatile uint8_t _dummy_byte;

// USCI_A1
typedef UCA<UCA1STAT, UCA1CTL0, UCA1CTL1,
            UCA1MCTL, UCA1BR0, UCA1BR1,
            UCA1RXBUF, UCA1TXBUF, UCA1ABCTL,
            UCA1IRTCTL, UCA1IRRCTL, UCA1IE,
            UCA1IFG, UCRXIE, UCTXIE, UCRXIFG, UCTXIFG,
            USCI_A1_VECTOR> USCI_A1;

// USCI_B1
typedef UCB<UCB1STAT, UCB1CTL0, UCB1CTL1,
            UCB1BR0, UCB1BR1, UCB1IE,
            UCB1RXBUF, UCB1TXBUF, UCB1I2COA,
            UCB1I2CSA, UCB1IFG, UCTXIFG, UCRXIFG,
            USCI_B1_VECTOR> USCI_B1;

// P1-P6, PJ (not all pins exist externally)
typedef Port<P1IN, P1OUT, P1DIR, P1SEL,
            _dummy_byte, P1REN, P1IFG, P1IES,
            P1IE> Port1;

typedef Port<P2IN, P2OUT, P2DIR, P2SEL, _dummy_byte, P2REN, P2IFG, P2IES,
             P2IE> Port2;

typedef Port<P3IN, P3OUT, P3DIR, P3SEL, _dummy_byte, P3REN,
             _dummy_byte, _dummy_byte, _dummy_byte> Port3;

typedef Port<P4IN, P4OUT, P4DIR, P4SEL, _dummy_byte, P4REN,
             _dummy_byte, _dummy_byte, _dummy_byte> Port4;

typedef Port<P5IN, P5OUT, P5DIR, P5SEL, _dummy_byte, P5REN,
             _dummy_byte, _dummy_byte, _dummy_byte> Port5;

typedef Port<P6IN, P6OUT, P6DIR, P6SEL, _dummy_byte, P6REN,
             _dummy_byte, _dummy_byte, _dummy_byte> Port6;

typedef Port<PJIN_L, PJOUT_L, PJDIR_L, _dummy_byte, _dummy_byte,
             PJREN_L, _dummy_byte, _dummy_byte, _dummy_byte> PortJ;

#endif // _F5510_H_
