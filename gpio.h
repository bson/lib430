#ifndef _GPIO_H_
#define _GPIO_H_

#include "common.h"
#include "accessors.h"

// Port 1,2 has all of registers, while P3-P7 lack the edge interrupt
// support and lack the last three.

template <volatile uint8_t& _IN,
          volatile uint8_t& _OUT,
          volatile uint8_t& _DIR,
          volatile uint8_t& _SEL,
          volatile uint8_t& _SEL2,
          volatile uint8_t& _REN,
          volatile uint8_t& _IFG,
          volatile uint8_t& _IES,
          volatile uint8_t& _IE>
class Port {
public:
    ACCESSOR(volatile uint8_t&, getPIN, _IN);
    ACCESSOR(volatile uint8_t&, getPOUT, _OUT);
    ACCESSOR(volatile uint8_t&, getPDIR, _DIR);
    ACCESSOR(volatile uint8_t&, getPSEL, _SEL);
    ACCESSOR(volatile uint8_t&, getPSEL2, _SEL2);
    ACCESSOR(volatile uint8_t&, getPREN, _REN);
    ACCESSOR(volatile uint8_t&, getPIFG, _IFG);
    ACCESSOR(volatile uint8_t&, getPIES, _IES);
    ACCESSOR(volatile uint8_t&, getPIE, _IE);

    Port() { }
};


// PIN abstraction, for convenience and to simplify pin swapping
// without having to prune significant bodies of code and change
// e.g. P1xxx to P2xxx.
template <typename PORT,
          uint8_t  _BIT>
class Pin: public PORT {
public:
    enum { MASK = 1 << _BIT };

    enum Direction {
        OUTPUT = MASK,
        INPUT  = 0
    };

    enum Select {
        SEL0 = 0b00,
        SEL1 = 0b01,
        SEL2 = 0b10,
        SEL3 = 0b11,
    };

    enum Pullup {
        ENABLE = MASK,
        DISABLE = 0
    };

    // Pin.IN gettor
    static force_inline uint8_t getIN() { return PORT.P_IN & MASK; }

    // Set pin state
    static force_inline void set(bool state) {
        PORT.P_OUT = (PORT.P_OUT & ~MASK) | (state ? MASK : 0);
    }

    // Configure
    static force_inline void config(Direction dir,
                                    Select sel,
                                    Pullup ren) {
        PORT.P_DIR  = (PORT.P_DIR  & ~MASK) | uint8_t(dir);
        PORT.P_SEL  = (PORT.P_SEL  & ~MASK) | (uint8_t(sel) & 1);
        PORT.P_SEL2 = (PORT.P_SEL2 & ~MASK) | ((uint8_t(sel) >> 1) & 1);
        PORT.P_REN  = (PORT.P_REN  & ~MASK) | uint8_t(ren);
    };
                               
};

#endif // _GPIO_H_
