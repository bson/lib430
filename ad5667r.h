#ifndef _AD5667R_H_
#define _AD5667R_H_

// This can also be used with variants without internal reference, as well as
// the 12- and 14-bit versions (AD5627 and AD5647, respectively).  For the
// shorter word size, use the HIGH bits.
//
// The cal table is a set of actual values for each bit, relative to bit 0.
// So for example, if bit 0 is 0.1300mV and bit 10 is 0.1045V, then the calibration
// value for bit 10 is 0.1045/0.00013 = 949.  This means bit 10 outputs 949 times
// that of bit 0.  Since the AD5667 series has two isotonous resistor networks, it
// means each output bit always contributes the same value and the actual value
// output is the sum of the voltages for all the bits.  So to output a specific
// value we re-calculate based on the calibration table exactly which sum of output
// bits produce the desired value.
// The first value in the table is bit 15 and the last is bit 0.
// If no calibration table has been installed values are output verbatim.  This is
// useful for calibration, by measuring the voltage of each bit separately.


#include <msp430.h>
#include <stdint.h>

namespace ad5667r {

template <typename Bus, typename Device>
class DAC: public Device {
    enum { NCHANNELS = 2 };

    uint16_t *cal_table[NCHANNELS];

public:
    DAC(Bus& bus, uint8_t addr)
        : Device(bus, addr),
          _cal_table(NULL, NULL) {
    }

    void probe() { Device::dummy_probe(); }

    // Install calibration table
    void set_cal(uint8_t output, uint16_t *table) { _cal_table[output] = table; }

    // Sychronously change both outputs
    void update(uint16_t v0, uint16_t v1);

protected:
    // Calibration correct value
    uint16_t cal_correct(uint8_t output, uint16_t value) const;

private:
    DAC(const DAC&);
    DAC& operator=(const DAC&);
};

}; // namespace ad5667r

#endif  // _AD5667R_H_
