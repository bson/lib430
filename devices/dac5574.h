#ifndef _DAC5574_H_
#define _DAC5574_H_

#include "common.h"

namespace dac5574 {

template <typename Device>
class DAC: public Device {
public:
    DAC(uint8_t addr)
        : Device(addr) {
    }

    void probe() { Device::dummy_probe(); }

    // Sychronously change all outputs
    void update_all(uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4);

protected:
    // Write control, msb, lsb (defaulting lsb to 0)
    void write_control(uint8_t control, uint8_t msb, uint8_t lsb = 0);

private:
    DAC(const DAC&);
    DAC& operator=(const DAC&);
};

}; // namespace dac5574

#endif	// _DAC5574_H_
