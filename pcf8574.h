#ifndef _PCF8574_H_
#define _PCF8574_H_

#include <stdint.h>
#include "i2c_master/i2c.h"


class Pcf8574: public I2CDevice {
public:
    Pcf8574(I2CBus& bus, uint8_t addr)
        : I2CDevice(bus, addr) {
    }

    void probe() { I2CDevice::dummy_probe(); }

    void set(uint8_t bits) { I2CDevice::transmit(bits); }
private:
    Pcf8574(const Pcf8574&);
    Pcf8574& operator=(const Pcf8574&);
};

#endif // _PCF8574_H_
