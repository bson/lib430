#ifndef _UART_H_
#define _UART_H_

#include "common.h"

template <typename USCI>
class Uart: public USCI {
    bool _nl;
public:
    Uart() : _nl(false) { }

    void init() {

        USCI.CTL1 |= msp430::SWRST;
        USCI.CTL1 &= ~msp430::SWRST;
        USCI.CTL1 = msp430::SSEL1; // SMCLK (0b10)

        USCI.STAT = 0;
    }
     
    // Parameters: rate, parity enable, even parity, 8 bits, 2 stop bits
    // Does not permit setting big-endian bit order, non-UART mode, or
    // synchronous mode.
    // Default is 19200,N,8,1,autocr
    void set_format(uint32_t bps = 19200, bool parity = false, 
                    bool evenpar = false, bool bits8 = true, 
                    bool spb2 = false, bool autocr = true) {
        uint8_t bits = USCI.CTL0 & ~(PEN|PAR|MSB|U7BIT|SPB|MODE0|MODE1|SYNC);
        if (parity) {
            bits |= PEN;
            if (even)  
                bits |= PAR;
        }
        if (spb2)
            bits |= SPB;
        if (!bits8)
            bits |= U7BIT;

        USCI.CTL0 = bits;
        
        const uint16_t rate = uint32_t(SMCLK)/bps;
        USCI.BR1  = rate >> 8;
        USCI.BR0  = rate & 0xff;
        USCI.MCTL = 0;

        _nl = autocr;
    }

    bool start_write(uint8_t data) { write(data); return true; }
    bool write(uint8_t data) {
        if (_nl && data == '\n') {
            write('\r');
        }
        while (!(USCI.IFG2 & USCI.TXIFG))
            ;
        USCI.TXBUF = data;
        USCI.IFG2 &= ~USCI.TXIFG;
    }
    bool write_done() { }

    void putc(char c) { write(c); }
    void puts(const char *s) {
        while (*s) 
            puts(*s++);
    }
    bool read_ready() { return USCI.IFG2 & USCI.RXIFG; }
    uint8_t read() { return USCI.RXBUF; }
};

#endif // _UART_H_
