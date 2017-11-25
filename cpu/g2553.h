#ifndef _G2553_H_
#define _G2553_H_

#include "common.h"
#include "timer.h"
#include "usci_b.h"

typedef UCB<msp430::UCB0STAT, msp430::UCB0CTL0, msp430::UCB0CTL1, msp430::UCB0BR0, msp430::UCB0BR1,
            msp430::UCB0I2CIE, msp430::UCB0RXBUF, msp430::UCB0TXBUF, msp430::UCB0I2COA, msp430::UCB0I2CSA,
            msp430::UC0IFG> UCB0;

typedef TimerA3<msp430::TA0CTL, msp430::TA0R, msp430::TA0CCTL0,
                msp430::TA0CCR0, msp430::TA0CCTL1, msp430::TA0CCR1,
                msp430::TA0CCTL2, msp430::TA0CCR2, msp430::TA0IV> TimerA3_0;

// System needs a timer someplace.
typedef TimerA3_0 SysTimer;
extern SysTimer _sysTimer;

#endif // _G2553_H_
