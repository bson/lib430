#ifndef _G2553_H_
#define _G2553_H_

#include "common.h"

#include "usci_b.h"

typedef UCB<msp430::UCB0STAT, msp430::UCB0CTL0, msp430::UCB0CTL1, msp430::UCB0BR0, msp430::UCB0BR1,
            msp430::UCB0I2CIE, msp430::UCB0RXBUF, msp430::UCB0TXBUF, msp430::UCB0I2COA, msp430::UCB0I2CSA,
            msp430::UC0IFG> UCB0;

#endif // _G2553_H_
