#ifdef _MAIN_

#include "common.h"
#include "ad5667r.h"

typedef unsigned int uint;

namespace ad5667r {

template <typename Bus, typename Device, int NBITS>
void DAC<Bus,Device,NBITS>::update(uint16_t v0, uint16_t v1) {
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

template <typename Bus, typename Device, int NBITS>
uint16_t DAC<Bus,Device,NBITS>::cal_correct(uint8_t output, uint16_t value) const {
    const uint32_t *table = _cal_data[output];
    if (!table) {
        return value;
    }
    uint32_t v = value << 16;
    uint16_t bits = 0;
    uint16_t mask = 1<<15;
    for (uint n = 0; n < (NBITS-1); ++n) {
        if (v >= table[n]) {
            bits |= mask;
            v -= table[n];
        }
        mask >>= 1;
    }
    return bits;
}

template <typename Bus, typename Device, int NBITS>
void DAC<Bus,Device,NBITS>::install_cal_table(uint8_t output, uint32_t *table) {
    // TODO - validate cal table.  Maybe put the checksum at the bit 0 position.

    _cal_table[output] = table;
    _bad_cal[output]   = false;
}

}; // namespace ad5667r

#endif // _MAIN_
