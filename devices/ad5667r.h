// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef _AD5667R_H_
#define _AD5667R_H_

// This can also be used with variants without internal reference, as well as
// the 12- and 14-bit versions (AD5627 and AD5647, respectively).  For the
// shorter word size, use the HIGH bits.
//
// The cal table is a set of actual values for each bit, relative to bit 0.
// So for example, if bit 0 is 0.1300mV and bit 10 is 0.1045V, then the calibration
// value for bit 10 is 0.1045/0.00013 = 949.  This means bit 10 outputs 949 times
// that of bit 0.  Since the AD5667 series has two resistor networks, it
// means each output bit always contributes the same value and the actual value
// output is the sum of the voltages for all the bits.  So to output a specific
// value we re-calculate based on the calibration table exactly which sum of output
// bits produce the desired value.  The values in the table are 32-bit fixed point
// quantities (Q16.16).
// The first value is for bit 15 and the last is bit 1.  Bit 0 is implicitly
// 1.0 (0x00010000) and is omitted from the table.
// If no calibration table has been installed values are output verbatim.  This is
// useful for calibration, by measuring the voltage of each bit separately.
// The inclusion of only 15 values means the size of the table is 15*4 = 60 bytes
// and the table plus a 32 bit checksum fits in a 64-byte flash or EEPROM sector.

#include "common.h"

// This assumes a grounded ADDR pin
#define AD5667R_ADDR (0xf)

namespace ad5667r {

template <typename Device, int NBITS=16>
class DAC: public Device {
    enum { NCHANNELS = 2 };

    const uint32_t *_cal_table[NCHANNELS];

public:
    enum Command {
    		WRITE        = 0b000 << 3,  // Write to Reg n
		UPDATE       = 0b001 << 3,  // Update DAC n
		WRITE_UPALL  = 0b010 << 3,  // Write to reg n, update all DACs
		WRITE_UPDATE = 0b011 << 3,  // Write to reg, update DAC n
    		LDAC         = 0b110 << 3,  // LDAC pin setup
    		RESET	     = 0b101 << 3,  // Software reset
    		POWER        = 0b100 << 3,  // Power up/down
		REFERENCE    = 0b111 << 3   // Internal reference enable/disable
    };

    DAC(uint8_t addr)
        : Device(addr) {
    		_cal_table[0] = _cal_table[1] = NULL;
    }

    void probe() {
    		if (Device::state() == Device::UNATTACHED) {
    			Device::dummy_probe();
    			if (Device::state() == Device::ATTACHED) {
    				init();
    			}
    		}
    }

    // Initialize
    void init();

    // Install calibration table
    void install_cal_table(uint8_t output, const uint32_t *table) {
        _cal_table[output] = table;
    }

    // True if running calibrated on both channels
    bool calibrated() const { return _cal_table[0] && _cal_table[1]; }

    // Synchronously change both outputs
    void update(uint16_t v0, uint16_t v1);

    // Update single output with 32-bit Q5.27 value
    void update32(uint8_t channel, uint32_t value);

    // Send command followed by 16 bits of parameters
    void command(uint8_t cmd, uint16_t data);

protected:
    // Calibration correct value
    uint16_t cal_correct(uint8_t output, uint32_t value) const;

private:
    DAC(const DAC&);
    DAC& operator=(const DAC&);
};

}; // namespace ad5667r

#endif  // _AD5667R_H_
