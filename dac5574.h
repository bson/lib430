#ifndef _DAC5574_H_
#define _DAC5574_H_

#include <msp430.h>
#include <stdint.h>
#include "i2c_master/i2c.h"

class Dac5574: public I2CDevice {
public:
    Dac5574(I2CBus& bus, uint8_t addr)
        : I2CDevice(bus, addr) {
    }

    void probe() { I2CDevice::dummy_probe(); }

    // Sychronously change all outputs
    void update_all(uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4);

protected:
    // Write control, msb, lsb (defaulting lsb to 0)
    void write_control(uint8_t control, uint8_t msb, uint8_t lsb = 0);

private:
    Dac5574(const Dac5574&);
    Dac5574& operator=(const Dac5574&);
};

#endif	// _DAC5574_H_
