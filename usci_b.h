#ifndef _USCI_B_H_
#define _USCI_B_H_

#include <msp430.h>
#include <stdint.h>
#include <stddef.h>

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
          volatile uint8_t& _IFG>
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
        SSEL_0 = UCSSEL_0,
        SSEL_1 = UCSSEL_1,
        SSEL_2 = UCSSEL_2,
        SSEL_3 = UCSSEL_3,

        LISTEN = UCLISTEN,
        RXERR = UCRXERR,
        ADDR = UCADDR,
        UBUSY = UCBUSY,
        IDLE = UCIDLE
    };

    volatile uint8_t& STAT;
    volatile uint8_t& CTL0;
    volatile uint8_t& CTL1;
    volatile uint8_t& BR0;
    volatile uint8_t& BR1;
    volatile uint8_t& I2CIE;
    volatile uint8_t& RXBUF;
    volatile uint8_t& TXBUF;
    volatile uint16_t& I2COA;
    volatile uint16_t& I2CSA;
    volatile uint8_t& IFG;

    UCB() 
        : STAT(_STAT),
          CTL0(_CTL0),
          CTL1(_CTL1),
          BR0(_BR0),
          BR1(_BR1),
          I2CIE(_I2CIE),
          RXBUF(_RXBUF),
          TXBUF(_TXBUF),
          I2COA(_I2COA),
          I2CSA(_I2CSA),
          IFG(_IFG) {
    }
};

#endif //_USCI_B_H_
