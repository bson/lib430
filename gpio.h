// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef _GPIO_H_
#define _GPIO_H_

#include "common.h"
#include "accessors.h"

// Port 1,2 has all of registers, while P3-P7 lack the edge interrupt
// support and so don't have those three registers.

template <volatile uint8_t& _IN,
          volatile uint8_t& _OUT,
          volatile uint8_t& _DIR,
          volatile uint8_t& _SEL,
          volatile uint8_t& _SEL2,
          volatile uint8_t& _REN,
          volatile uint8_t& _DS,
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
    ACCESSOR(volatile uint8_t&, getPDS, _DS);
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
class Pin {
public:
    // This permits accessing the underlying GPIO port directly
    typedef PORT Port;

    enum {
        MASK = 1 << _BIT,
        BIT  = _BIT
    };

    enum Direction {
        OUTPUT = MASK,
        INPUT  = 0,
        MODULE = 0
    };

    enum Select {
        SEL0 = 0b00,
        SEL1 = 0b01,
        SEL2 = 0b10,
        SEL3 = 0b11,
        IO_PIN = SEL0
    };

    enum Pullup {
        NO_RESISTOR   = 0,
        PULLUP = 1,
		PULLDOWN = 2
    };

    enum Edge {
    		RISE = 0,
		FALL = MASK
    };

    enum DriveStrength {
        LOW = 0,
        HIGH = MASK
    };

    // Pin.IN gettor
    static force_inline uint8_t getIN() { return PORT::P_IN & MASK; }

    // Set pin state
    static force_inline void set(bool state) {
        PORT::P_OUT = (PORT::P_OUT & ~MASK) | (state ? MASK : 0);
    }

    // Get state
    static force_inline bool get() {
    		return PORT::P_IN & MASK;
    }

    // Configure
    static void config(Direction dir,
    				      Select sel = IO_PIN,
				      Pullup ren = NO_RESISTOR) {
        PORT::P_DIR  = (PORT::P_DIR  & ~MASK) | uint8_t(dir);
        PORT::P_SEL  = (PORT::P_SEL  & ~MASK) | ((uint8_t(sel) & 1) << _BIT);
        PORT::P_SEL2 = (PORT::P_SEL2 & ~MASK) | (((uint8_t(sel) >> 1) & 1) << _BIT);
        if (sel != IO_PIN) {
            ren = NO_RESISTOR;
        }
        PORT::P_REN  = (PORT::P_REN  & ~MASK) | (ren ? MASK : 0);
        if (ren && dir == INPUT) {
        		PORT::P_OUT = (PORT::P_OUT & ~MASK) | (ren == PULLUP ? MASK : 0);
        }
    };

    // Enable pin interrupt
    static force_inline void enable_int(Edge edge) {
    		PORT::P_IES = (PORT::P_IES & ~MASK) | uint8_t(edge);
    		PORT::P_IFG &= ~MASK;
    		PORT::P_IE  |= MASK;
    };

    // Simply enable without affecting edge sense
    static force_inline void enable_int() {
    		PORT::P_IE  |= MASK;
    }

    // Disable pin interrupt
    static force_inline void disable_int() {
    		PORT::P_IE  &= ~MASK;
    };

    // Check for interrupt flag
    static force_inline bool pending_int() {
    		return PORT::P_IFG & MASK;
    }

    // Toggle edge interrupt sense
    static force_inline void toggle_edge() {
    		PORT::P_IES ^= MASK;
    }

    // Acknowledge interrupt
    static force_inline void ack_int() {
    		PORT::P_IFG &= ~MASK;
    };

    // Make input
    static force_inline void make_input() {
        PORT::P_DIR &= ~MASK;
    }

    // Make output
    static force_inline void make_output() {
        PORT::P_DIR |= MASK;
    }

    // Set drive strength
    static force_inline void set_drive(DriveStrength ds) {
        PORT::P_DS = (PORT::P_DS & ~MASK) | uint8_t(ds);
    }
};

// Dummy no-op pin for uses where no pin is needed.
class NoPin {
public:
    enum { MASK = 1 };

    enum Direction {
        OUTPUT = MASK,
        INPUT  = 0,
        MODULE = 0
    };

    enum Select {
        SEL0 = 0b00,
        SEL1 = 0b01,
        SEL2 = 0b10,
        SEL3 = 0b11,
        IO_PIN = SEL0
    };

    enum Pullup {
        PULL_RESISTOR = MASK,
        NO_RESISTOR   = 0
    };

    enum Edge {
    		RISE = 0,
		FALL = MASK
    };

    enum DriveStrength {
        LOW = 0,
        HIGH = MASK
    };

    static force_inline uint8_t getIN() { return 0; }
    static force_inline void set(bool state) { }
    static force_inline void config(Direction dir,
                                    Select sel,
                                    Pullup ren = NO_RESISTOR) { }
    static force_inline void enable_int(Edge edge) { }
    static force_inline void disable_int() { }
    static force_inline bool pending_int() { return false; }
    static force_inline void toggle_edge() { }
    static force_inline void ack_int() { }
    static force_inline void set_drive(DriveStrength ds) { }
};
#endif // _GPIO_H_
