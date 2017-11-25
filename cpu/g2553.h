#ifndef _G2553_H_
#define _G2553_H_

#include "common.h"
#include "timer.h"
#include "usci_b.h"
#include "gpio.h"

typedef UCB<msp430::UCB0STAT, msp430::UCB0CTL0, msp430::UCB0CTL1, 
            msp430::UCB0BR0, msp430::UCB0BR1, msp430::UCB0I2CIE, 
            msp430::UCB0RXBUF, msp430::UCB0TXBUF, msp430::UCB0I2COA, 
            msp430::UCB0I2CSA, msp430::UC0IFG> UCB0;

typedef TimerA3<msp430::TA0CTL, msp430::TA0R, msp430::TA0CCTL0,
                msp430::TA0CCR0, msp430::TA0CCTL1, msp430::TA0CCR1,
                msp430::TA0CCTL2, msp430::TA0CCR2, msp430::TA0IV> TimerA3_0;

// System needs a timer someplace.
typedef TimerA3_0 SysTimer;
extern SysTimer _sysTimer;

typedef Port<msp430::P1IN, msp430::P1OUT, msp430::P1DIR, msp430::P1SEL,
             msp430::P1SEL2, msp430::P1REN, msp430::P1IFG, msp430::P1IES,
             msp430::P1IE> Port1;

typedef Port<msp430::P2IN, msp430::P2OUT, msp430::P2DIR, msp430::P2SEL,
             msp430::P2SEL2, msp430::P2REN, msp430::P2IFG, msp430::P2IES,
             msp430::P2IE> Port2;

extern volatile uint8_t _dummy_byte;

// 28 pin devices only
typedef Port<msp430::P3IN, msp430::P3OUT, msp430::P3DIR, msp430::P3SEL,
            msp430::P3SEL2, msp430::P3REN, _dummy_byte, _dummy_byte,
            _dummy_byte> Port3;

#endif // _G2553_H_
