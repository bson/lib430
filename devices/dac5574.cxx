#include "common.h"
#include "dac5574.h"

namespace dac5574 {

template <typename Bus, typename Device>
void DAC<Bus,Device>::write_control(uint8_t control, uint8_t msb, uint8_t lsb) {
    if (Device::start_write(control)) {
        Device::write(msb);
        Device::write(lsb);
        Device::write_done();
    }
}

template <typename Bus, typename Device>
void DAC<Bus,Device>::update_all(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3) {
    write_control(0b00000000, v0);
    write_control(0b00000010, v1);
    write_control(0b00000100, v2);
    write_control(0b00100110, v3);
}

};
