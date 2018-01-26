#ifndef _UART_H_
#define _UART_H_

#include "common.h"
#include "util/deque.h"

template <typename USCI>
class Uart {
#ifdef UART_TX_BUF
    Deque<uint8_t, UART_TX_BUF> _txbuf;
#endif
#ifdef UART_RX_BUF
    Deque<uint8_t, UART_RX_BUF> _rxbuf;
#endif
    bool _nl;
public:
    enum { INTVEC = USCI::INTVEC };

    Uart() : _nl(false) { }

    void init() {
        USCI::CTL1 |= USCI::SWRST;
        USCI::CTL1 &= ~USCI::SWRST;
        USCI::CTL1 = USCI::SSEL1; // SMCLK (0b10)

        USCI::STAT = 0;
        USCI::CPU_IE2 &= ~(USCI::TXIE | USCI::RXIE);
#ifdef UART_RX_BUF
        USCI::CPU_IE2 |= USCI::RXIE;
#endif
        USCI::CPU_IFG2 &= ~USCI::RXIFG;  // Discard anything in receiver
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
#ifdef UART_TX_BUF
        // If interrupts are disabled, then the transmitter is idle.  Just put the
        // byte in the transmitter and enable interrupts.
        if (!(USCI::CPU_IE2 & USCI::TXIE)) {
            USCI::TXBUF = data;
            USCI::CPU_IFG2 &= ~USCI::TXIFG;
            USCI::CPU_IE2 |= USCI::TXIE;
            return true;
        }
        // Otherwise add it to the buffer
        if (_txbuf.space()) {
            _txbuf.push_back(data);
        }
        return true;
#else // POLLED
        while (!(USCI::CPU_IFG2 & USCI::TXIFG))
            ;
        USCI::TXBUF = data;
        USCI::CPU_IFG2 &= ~USCI::TXIFG;
        return true;
#endif
    }
    bool write_done() { return true; }

#ifdef UART_RX_BUF
    bool read_ready() { return !_rxbuf.empty(); }
    uint8_t read() {
        if (_rxbuf.empty())
            return 0;
        else
            return _rxbuf.pop_front();
    }
#else // POLLED
    bool read_ready() { return USCI::CPU_IFG2 & USCI::RXIFG; }
    uint8_t read() { return USCI::RXBUF; }
#endif

    void putc(char c) { write(c); }
    void puts(const char *s) {
        while (*s) 
            putc(*s++);
    }
    void putln(const char* s) {
    		puts(s);
    		putc('\n');
    }

    // ISR
    void isr() {
#ifdef UART_TX_BUF
        if (USCI::CPU_IFG2 & USCI::TXIFG) {
            if (_txbuf.empty()) {
                USCI::CPU_IE2 &= ~USCI::TXIE;
            } else {
                USCI::TXBUF = _txbuf.pop_front();
            }
        }
#endif
#ifdef UART_RX_BUF
        if (USCI::CPU_IFG2 & USCI::RXIFG) {
            if (_rxbuf.space()) {
                _rxbuf.push_back(const_cast<const uint8_t&>(USCI::RXBUF));
            } else {
                (void)USCI::RXBUF; // XXX does USCI require reading to clear IFG?
            }
        }
#endif
    }

};

#endif // _UART_H_
