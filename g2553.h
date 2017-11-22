#ifndef _G2553_H_
#define _G2553_H_

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
        // UART-Mode Bits
        PEN = UCPEN,
        PAR = UCPAR,
        MSB = UCMSB,
        U7BIT = UC7BIT,
        SPB = UCSPB,
        MODE1 = UCMODE1,
        MODE0 = UCMODE0,
        SYNC = UCSYNC,

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

        // UART-Mode Bits
        SSEL1 = UCSSEL1,
        SSEL0 = UCSSEL0,
        RXEIE = UCRXEIE,
        BRKIE = UCBRKIE,
        DORM = UCDORM,
        TXADDR = UCTXADDR,
        TXBRK = UCTXBRK,
        SWRST = UCSWRST,

        // I2C-Mode Bits
        TR = UCTR,
        TXNACK = UCTXNACK,
        TXSTP = UCTXSTP,
        TXSTT = UCTXSTT,
        SSEL_0 = UCSSEL_0,
        SSEL_1 = UCSSEL_1,
        SSEL_2 = UCSSEL_2,
        SSEL_3 = UCSSEL_3,

        BRF3 = UCBRF3,
        BRF2 = UCBRF2,
        BRF1 = UCBRF1,
        BRF0 = UCBRF0,
        BRS2 = UCBRS2,
        BRS1 = UCBRS1,
        BRS0 = UCBRS0,
        OS16 = UCOS16,

        BRF_0 = UCBRF_0,
        BRF_1 = UCBRF_1,
        BRF_2 = UCBRF_2,
        BRF_3 = UCBRF_3,
        BRF_4 = UCBRF_4,
        BRF_5 = UCBRF_5,
        BRF_6 = UCBRF_6,
        BRF_7 = UCBRF_7,
        BRF_8 = UCBRF_8,
        BRF_9 = UCBRF_9,
        BRF_10 = UCBRF_10,
        BRF_11 = UCBRF_11,
        BRF_12 = UCBRF_12,
        BRF_13 = UCBRF_13,
        BRF_14 = UCBRF_14,
        BRF_15 = UCBRF_15,

        BRS_0 = UCBRS_0,
        BRS_1 = UCBRS_1,
        BRS_2 = UCBRS_2,
        BRS_3 = UCBRS_3,
        BRS_4 = UCBRS_4,
        BRS_5 = UCBRS_5,
        BRS_6 = UCBRS_6,
        BRS_7 = UCBRS_7,

        LISTEN = UCLISTEN,
        FE = UCFE,
        OE = UCOE,
        PE = UCPE,
        BRK = UCBRK,
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

typedef UCB<UCB0STAT, UCB0CTL0, UCB0CTL1, UCB0BR0, UCB0BR1,
            UCB0I2CIE, UCB0RXBUF, UCB0TXBUF, UCB0I2COA, UCB0I2CSA,
            UC0IFG> UCB0;

#endif // _G2553_H_
