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
