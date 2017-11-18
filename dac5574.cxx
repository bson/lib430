#include <msp430.h>
#include <stdint.h>
#include "dac5574.h"


void Dac5574::write_control(uint8_t control, uint8_t msb, uint8_t lsb) {
    if (I2CDevice::start_write(control)) {
        I2CDevice::write(msb);
        I2CDevice::write(lsb);
        I2CDevice::write_done();
    }
}

void Dac5574::update_all(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3) {
    write_control(0b00000000, v0);
    write_control(0b00000010, v1);
    write_control(0b00000100, v2);
    write_control(0b00100110, v3);
}
