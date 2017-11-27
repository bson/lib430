#ifndef _UART_H_
#define _UART_H_

#include "common.h"

template <typename USCI>
class Uart {
    bool _nl;
public:
    Uart() : _nl(false) { }

    void init() {
        USCI::CTL1 |= USCI::SWRST;
        USCI::CTL1 &= ~USCI::SWRST;
        USCI::CTL1 = USCI::SSEL1; // SMCLK (0b10)

        USCI::STAT = 0;
    }
     
    // Parameters: rate, parity enable, even parity, 8 bits, 2 stop bits
    // Does not permit setting big-endian bit order, non-UART mode, or
    // synchronous mode.
    // Default is 19200,N,8,1,autocr
    void set_format(uint32_t bps = 19200, bool parity = false, 
                    bool evenpar = false, bool bits8 = true, 
                    bool spb2 = false, bool autocr = true) {
        uint8_t bits = USCI::CTL0 & ~(USCI::PEN|USCI::PAR|USCI::MSB|USCI::U7BIT|
                                        USCI::SPB|USCI::MODE0|USCI::MODE1|USCI::SYNC);
        if (parity) {
            bits |= USCI::PEN;
            if (evenpar)
                bits |= USCI::PAR;
        }
        if (spb2)
            bits |= USCI::SPB;
        if (!bits8)
            bits |= USCI::U7BIT;

        USCI::CTL0 = bits;
        
        const uint16_t rate = uint32_t(SMCLK)/bps;
        USCI::BR1  = rate >> 8;
        USCI::BR0  = rate & 0xff;
        USCI::MCTL = 0;

        _nl = autocr;
    }

    bool start_write(uint8_t data) { write(data); return true; }
    bool write(uint8_t data) {
        if (_nl && data == '\n') {
            write('\r');
        }
        while (!(USCI::CPU_IFG2 & USCI::TXIFG))
            ;
        USCI::TXBUF = data;
        USCI::CPU_IFG2 &= ~USCI::TXIFG;
        return true;
    }
    bool write_done() { return true; }

    void putc(char c) { write(c); }
    void puts(const char *s) {
        while (*s) 
            putc(*s++);
    }
    bool read_ready() { return USCI::CPU_IFG2 & USCI::RXIFG; }
    uint8_t read() { return USCI::RXBUF; }
};

#endif // _UART_H_
