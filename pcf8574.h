#ifndef _PCF8574_H_
#define _PCF8574_H_

#include <stdint.h>

namespace pcf8574 {

template <typename Bus, typename Device>
class Expander: public Device {
public:
    Expander(Bus& bus, uint8_t addr)
        : Device(bus, addr) {
    }

    void probe() { Device::dummy_probe(); }

    void set(uint8_t bits) { Device::transmit(bits); }
private:
    Expander(const Expander&);
    Expander& operator=(const Expander&);
};

};

#endif // _PCF8574_H_
