#ifndef _USCI_A_H_
#define _USCI_A_H_

#include <msp430.h>
#include <stdint.h>
#include <stddef.h>

#include "accessors.h"

template <volatile uint8_t& _STAT,
          volatile uint8_t& _CTL0,
          volatile uint8_t& _CTL1,
          volatile uint8_t& _MCTL,
          volatile uint8_t& _BR0,
          volatile uint8_t& _BR1,
          volatile uint8_t& _RXBUF,
          volatile uint8_t& _TXBUF,
          volatile uint8_t& _ABCTL,
          volatile uint8_t& _IRTCTL,
          volatile uint8_t& _IRRCTL,
          volatile uint8_t& _IE2,
          volatile uint8_t& _IFG2,
          uint8_t _RXIFG,
          uint8_t _TXIFG>
class UCA0 {
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

        // UART-Mode Bits
        SSEL1 = UCSSEL1,
        SSEL0 = UCSSEL0,
        RXEIE = UCRXEIE,
        BRKIE = UCBRKIE,
        DORM = UCDORM,
        TXBRK = UCTXBRK,
        SWRST = UCSWRST,

        LISTEN = UCLISTEN,
        FE = UCFE,
        OE = UCOE,
        PE = UCPE,
        BRK = UCBRK,
        RXERR = UCRXERR,
        UBUSY = UCBUSY,
        IDLE = UCIDLE,

        RXIFG = _RXIFG,
        TXIFG = _TXIFG
    };

    ACCESSOR(volatile uint8_t&, getSTAT, _STAT);
    ACCESSOR(volatile uint8_t&, getCTL0, _CTL0);
    ACCESSOR(volatile uint8_t&, getCTL1, _CTL1);
    ACCESSOR(volatile uint8_t&, getMCTL, _MCTL);
    ACCESSOR(volatile uint8_t&, getBR0, _BR0);
    ACCESSOR(volatile uint8_t&, getBR1, _BR1);
    ACCESSOR(volatile uint8_t&, getRXBUF, _RXBUF);
    ACCESSOR(volatile uint8_t&, getTXBUF, _TXBUF);
    ACCESSOR(volatile uint8_t&, getABCTL, _ABCTL);
    ACCESSOR(volatile uint8_t&, getIRTCTL, _IRTCTL);
    ACCESSOR(volatile uint8_t&, getIRRCTL, _IRRCTL);
    ACCESSOR(volatile uint8_t&, getIE2, _IE2);
    ACCESSOR(volatile uint8_t&, getIFG2, _IFG2);

    UCA() { ; }
};

typedef UCA<UCA0STAT, UCA0CTL0, UCA0CTL1, UCA0MCTL, 
            UCA0BR0, UCA0BR1, UCA0RXBUF, UCA0TXBUF,
            UCA0ABCTL, UCA0IRTCTL, UCA0IRRCTL,
            IE2, IFG2, UCA0RXIFG, UCA0TXIFG> UCA0;

#endif //_USCI_A_H_
