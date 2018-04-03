// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef _USCI_B_H_
#define _USCI_B_H_

#include <stdint.h>
#include <stddef.h>

#include "common.h"
#include "accessors.h"

template <volatile uint8_t& _STAT,
          volatile uint8_t& _CTL0,
          volatile uint8_t& _CTL1,
          volatile uint8_t& _BR0,
          volatile uint8_t& _BR1,
          volatile uint8_t& _I2CIE,
          volatile uint8_t& _RXBUF,
          volatile uint8_t& _TXBUF,
          volatile uint16_t& _I2COA,
          volatile uint16_t& _I2CSA,
          volatile uint8_t& _IFG,
          uint8_t _TXIFG,
          uint8_t _RXIFG,
          uint16_t _VECTOR>
class UCB {
public:
    enum {
        // SPI-Mode Bits
        CKPH = UCCKPH,
        CKPL = UCCKPL,
        MST = UCMST,

        // I2C-Mode Bits
        A10 = UCA10,
        SLA10 = UCSLA10,
        MM = UCMM,
        MODE_0 = UCMODE_0,
        MODE_1 = UCMODE_1,
        MODE_2 = UCMODE_2,
        MODE_3 = UCMODE_3,

        // I2C-Mode Bits
        TR = UCTR,
        TXNACK = UCTXNACK,
        TXSTP = UCTXSTP,
        TXSTT = UCTXSTT,
        SSEL_ACLK = UCSSEL__ACLK,
        SSEL_SMCLK = UCSSEL__SMCLK,
        SWRST = UCSWRST,
        SYNC = UCSYNC,

        LISTEN = UCLISTEN,
        RXERR = UCRXERR,
        ADDR = UCADDR,
        UBUSY = UCBUSY,
        IDLE = UCIDLE,

        NACKIFG = UCNACKIFG,

        TXIFG = _TXIFG,
        RXIFG = _RXIFG,

        INTVEC = _VECTOR
    };

    ACCESSOR(volatile uint8_t&, getSTAT, _STAT);
    ACCESSOR(volatile uint8_t&, getCTL0, _CTL0);
    ACCESSOR(volatile uint8_t&, getCTL1, _CTL1);
    ACCESSOR(volatile uint8_t&, getBR0, _BR0);
    ACCESSOR(volatile uint8_t&, getBR1, _BR1);
    ACCESSOR(volatile uint8_t&, getI2CIE, _I2CIE);
    ACCESSOR(volatile uint8_t&, getRXBUF, _RXBUF);
    ACCESSOR(volatile uint8_t&, getTXBUF, _TXBUF);
    ACCESSOR(volatile uint16_t&, getI2COA, _I2COA);
    ACCESSOR(volatile uint16_t&, getI2CSA, _I2CSA);
    ACCESSOR(volatile uint8_t&, getIFG, _IFG);
    
    UCB() { ; }
};

#endif //_USCI_B_H_
