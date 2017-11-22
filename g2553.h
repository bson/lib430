#ifndef _G2553_H_
#define _G2553_H_

#include <msp430.h>
#include <stdint.h>
#include <stddef.h>

#include "usci_b.h"

typedef UCB<UCB0STAT, UCB0CTL0, UCB0CTL1, UCB0BR0, UCB0BR1,
            UCB0I2CIE, UCB0RXBUF, UCB0TXBUF, UCB0I2COA, UCB0I2CSA,
            UC0IFG> UCB0;

#endif // _G2553_H_
