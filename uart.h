// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef _UART_H_
#define _UART_H_

#include "common.h"
#include "util/deque.h"
#include "task.h"

template <typename USCI>
class Uart {
#ifdef UART_TX_BUF
    Deque<uint8_t, UART_TX_BUF> _txbuf;
#endif
#ifdef UART_RX_BUF
    Deque<uint8_t, UART_RX_BUF> _rxbuf;
#endif
#ifdef UART_TX_BUF
    volatile bool _txbusy;   // Transmission in progress
#endif
    bool _nl;
public:
    enum { INTVEC = USCI::INTVEC };

    Uart() : _nl(false) { }

    void init() {
        USCI::CTL1 |= USCI::SWRST;
        USCI::CTL1 &= ~USCI::SWRST;
#ifdef UART_SOURCE
        USCI::CTL1 = USCI::UART_SOURCE;
#else
        USCI::CTL1 = USCI::SSEL_ACLK;
#endif
        USCI::STAT = 0;
        USCI::CPU_IE2 &= ~(USCI::TXIE | USCI::RXIE);
#ifdef UART_RX_BUF
        _rxbuf.clear();
        USCI::CPU_IE2 |= USCI::RXIE;
#endif
        USCI::CPU_IFG2 &= ~USCI::RXIFG;  // Discard anything in receiver
#ifdef UART_TX_BUF
        _txbuf.clear();
        USCI::CPU_IE2 |= USCI::TXIE;
        USCI::CPU_IFG2 &= ~USCI::TXIFG;  // Discard anything in transmitter
        _txbusy = false;
#endif
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
#ifdef UART_TX_BUF
        // Just add byte directly to transmitter if not transmitting
        if (!_txbusy) {
            USCI::TXBUF = data;
            _txbusy = true;
        } else {
            // Otherwise add it to the buffer
            while (!_txbuf.space())
                Task::wait(Task::WChan(this));

            _txbuf.push_back(data);
        }
        return true;
#else // POLLED
        while (!(USCI::CPU_IFG2 & USCI::TXIFG))
            ;
        USCI::TXBUF = data;
        return true;
#endif
    }
    bool write_done() { return true; }

#ifdef UART_RX_BUF
    bool read_ready() { return !_rxbuf.empty(); }
    uint8_t read() {
        // ISR modifies _tail; we modify _head.  So no need to block interrupts.
        if (_rxbuf.empty())
            return ~0;

        return _rxbuf.pop_front();
    }
#else // POLLED
    bool read_ready() { return USCI::CPU_IFG2 & USCI::RXIFG; }
    uint8_t read() { return USCI::RXBUF; }
#endif

    void putc(char c) {
        if (_nl && c == '\n') {
            write('\r');
        }
        write(c);
    }
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
                USCI::CPU_IFG2 &= ~USCI::TXIFG;
                _txbusy = false;
            } else {
                USCI::TXBUF = _txbuf.pop_front();
            }
            Task::signal(Task::WChan(this));
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

    // Check if TX is busy
#ifdef UART_TX_BUF
    bool txbusy() { return _txbusy; }
#else
    bool txbusy() { return false; }
#endif
};

#endif // _UART_H_
