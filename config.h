// Note!  This is needed to compile and create this project as the static library lib430.lib.
// I'd recommend instead having a config.h in the main application project and including the
// source files needed directly, effectively recompiling them into the application from source.
// Just clone this file and edit.

#ifndef _CONFIG_H_
#define _CONFIG_H_

enum {
    DCO_CLOCK = 16000000,      // Oscillator frequency
    SMCLK     = DCO_CLOCK/8,   // SMCLK usable for peripherals = DCO/8 = 2MHz
    MCLK      = DCO_CLOCK      // Main CPU clock = DCO = 16MHz
};

// SCL is set by the slowest device
enum {
    I2C_BUS_SPEED = 100000
};

typedef unsigned int uint;


#endif // _CONFIG_H_
