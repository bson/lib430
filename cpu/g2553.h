// Copyright (c) 2020 Jan Brittenson
// See LICENSE for details.

#ifndef _G2553_H_
#define _G2553_H_

#include "common.h"
#include "timer.h"
#include "usci_a.h"
#include "usci_b.h"
#include "gpio.h"

typedef TimerA3<TA0CTL, TA0R, TA0CCTL0,
                TA0CCR0, TA0CCTL1, TA0CCR1,
                TA0CCTL2, TA0CCR2, TA0IV,
                TIMER0_A0_VECTOR, TIMER0_A1_VECTOR> TimerA3_0;

// System needs a timer someplace.
typedef TimerA3_0 SysTimer;

typedef UCA<UCA0STAT, UCA0CTL0, UCA0CTL1,
            UCA0MCTL, UCA0BR0, UCA0BR1,
            UCA0RXBUF, UCA0TXBUF, UCA0ABCTL,
            UCA0IRTCTL, UCA0IRRCTL, IE2,
            IFG2, UCA0RXIE, UCA0TXIE, UCA0RXIFG, UCA0TXIFG> UCA0;

typedef UCB<UCB0STAT, UCB0CTL0, UCB0CTL1,
            UCB0BR0, UCB0BR1, UCB0I2CIE,
            UCB0RXBUF, UCB0TXBUF, UCB0I2COA,
            UCB0I2CSA, UC0IFG, UCB0TXIFG, UCB0RXIFG> UCB0;

typedef Port<P1IN, P1OUT, P1DIR, P1SEL,
             P1SEL2, P1REN, P1DS, P1IFG, P1IES,
             P1IE> Port1;

typedef Port<P2IN, P2OUT, P2DIR, P2SEL,
             P2SEL2, P2REN, P2DS, P2IFG, P2IES,
             P2IE> Port2;

extern volatile uint8_t _dummy_byte;

// 28 pin devices only
typedef Port<P3IN, P3OUT, P3DIR, P3SEL,
            P3SEL2, P3REN, _dummy_byte, _dummy_byte,
            _dummy_byte> Port3;

#endif // _G2553_H_
