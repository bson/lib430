#ifdef _MAIN_

#include "common.h"
#include "ad5667r.h"

typedef unsigned int uint;

namespace ad5667r {

template <typename Device, int NBITS>
void DAC<Device,NBITS>::command(uint8_t cmd, uint16_t data) {
	if (Device::start_write(cmd)) {
		Device::write(data >> 8);
		Device::write(data & 0xff);
		Device::write_done();
	}
}

template <typename Device, int NBITS>
void DAC<Device,NBITS>::init() {
	command(POWER, 0b000011);
	command(REFERENCE, 1);
	command(LDAC, 0b00);
	command(WRITE_UPALL | 0b111, 0);  // Write 0 to both regs, update DACs
}

template <typename Device, int NBITS>
void DAC<Device,NBITS>::update(uint16_t v0, uint16_t v1) {
    v0 = cal_correct(0, uint32_t(v0) << 16);
    v1 = cal_correct(1, uint32_t(v1) << 16);

    if (Device::start_write(WRITE | 0)) {
        Device::write(v0 >> 8);
        Device::write(v0 & 0xff);
        Device::write(WRITE_UPALL | 1);
        Device::write(v1 >> 8);
        Device::write(v1 & 0xff);
        Device::write_done();
    }
}

template <typename Device, int NBITS>
void DAC<Device,NBITS>::update32(uint8_t channel, uint32_t value) {
	const uint16_t v = cal_correct(channel, value);
    if (Device::start_write(WRITE_UPALL | 0)) {
        Device::write(v >> 8);
        Device::write(v & 0xff);
        Device::write_done();
    }
}

template <typename Device, int NBITS>
uint16_t DAC<Device,NBITS>::cal_correct(uint8_t output, uint32_t value) const {
    const uint32_t *table = _cal_table[output];
    if (!table) {
        return value >> 16;
    }
    uint32_t v = value /* - min(v, table[0]) */;
    uint16_t bits = 0;
    uint16_t mask = 1U << 15;
    for (uint n = 1; n <= NBITS; ++n) {
        if (v >= table[n]) {
            bits |= mask;
            v -= table[n];
        }
        mask >>= 1;
    }
    return bits;
}

}; // namespace ad5667r

#endif // _MAIN_
