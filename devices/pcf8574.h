#ifndef _PCF8574_H_
#define _PCF8574_H_

#include "common.h"

#define PCF8574_ADDR(A0,A1,A2) \
	(0x20 | (A0) | ((A1) << 1) || ((A2) << 2))

#define PCF8574A_ADDR(A0,A1,A2) \
	(0x38 | (A0) | ((A1) << 1) || ((A2) << 2))

namespace pcf8574 {

template <typename Bus, typename Device>
class Expander: public Device {
public:
    Expander(Bus& bus, uint8_t addr)
        : Device(bus, addr) {
    }

    void probe() { Device::dummy_probe(); }

    void init() { }

    void set(uint8_t bits) { Device::transmit(bits); }
private:
    Expander(const Expander&);
    Expander& operator=(const Expander&);
};

};

#endif // _PCF8574_H_
