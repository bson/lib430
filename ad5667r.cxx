#include <msp430.h>
#include <stdint.h>
#include "ad5667r.h"

typedef unsigned int uint;

namespace ad5667r {

template <typename Bus, typename Device>
void DAC<Bus,Device>::update(uint16_t v0, uint16_t v1) {
    v0 = cal_correct(0, v0);
    v1 = cal_correct(1, v1);

    if (Device::start_write(0b00000000)) {
        Device::write(v0 >> 8)
        Device::write(v0 & 0xff);
        Device::write(0b00010001);
        Device::write(v1 >> 8);
        Device::write(v1 & 0xff);
        Device::write_done();
    }
}

template <typename Bus, typename Device>
uint16_t DAC<Bus,Device>::cal_correct(uint8_t output, uint16_t value) const {
    const uint16_t *table = _cal_data[output];
    if (!table) {
        return value;
    }
    uint16_t bits = 0;
    uint16_t mask = 1<<15;
    for (uint n = 0; n < 15 && value; ++n) {
        if (value >= table[n]) {
            bits |= mask;
            value -= table[n];
        }
        mask >>= 1;
    }
    return bits;
}

}; // namespace ad5667r
