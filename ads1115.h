#ifndef _ADS1115_H_
#define _ADS1115_H_

#include <msp430.h>
#include <stdint.h>

namespace ads1115 {

template <typename Bus, typename Device>
class ADC: public Device {
private:
    uint16_t _config;

public:
    enum {
        // Registers (comparator is not used by this code)
        REG_CONV = 0,   // Conversion register
        REG_CONF = 1,   // Config register
        REG_LO_THR = 2, // Comparator low threshold
        REG_HI_THR = 3, // Comparator high threshold

        // Channels
        IN0 = 0,
        IN1 = 1,
        IN2 = 2,
        IN3 = 3,

        // PGA settings (Full Scale Range)
        FSR_6_144 = 0,   // 6.144V
        FSR_4_096 = 1,
        FSR_2_048 = 2,
        FSR_1_024 = 3,
        FSR_0_512 = 4,
        FSR_0_256 = 5,  // 0.256V for values 5,6,7

        // Sample rates
        SPS_8 = 0,
        SPS_16 = 1,
        SPS_32 = 2,
        SPS_64 = 3,
        SPS_128 = 4,
        SPS_250 = 5,
        SPS_475 = 6,
        SPS_860 = 7
    };

    ADC(Bus& bus, uint8_t addr)
        : Device(bus, addr),
          _config(0) {
    }

    void probe() { Device::dummy_probe(); }

    // Select input, gain, and SPS
    void config(uint8_t channel, uint8_t fsr, uint8_t sps) {
        _config = ((_channel | 0b100) << 12) | (fsr << 9) | (_sps << 5);
    }

    // One-shot read
    void start_single_read();

private:
    ADC(const ADC&);
    ADC& operator=(const ADC&);
};

}; // namespace ads1115

#endif  // _ADS1115_H_
