#ifndef _ADS1115_H_
#define _ADS1115_H_

#include "common.h"

// This assumes a grounded ADDR pin
#define ADS1115_ADDR (0x48)

namespace ads1115 {

template <typename Device>
class ADC: public Device {
public:
    enum Channel {
        // Channels
        IN0 = 0,
        IN1 = 1,
        IN2 = 2,
        IN3 = 3
    };

    enum FSR {
        // PGA settings (Full Scale Range)
        FSR_6_144 = 0,   // 6.144V
        FSR_4_096 = 1,
        FSR_2_048 = 2,
        FSR_1_024 = 3,
        FSR_0_512 = 4,
        FSR_0_256 = 5  // 0.256V for values 5,6,7
    };

    enum SPS {
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

private:
    uint16_t _config;
    SPS _sps;

public:
    enum {
        // Registers (comparator is not used by this code)
        REG_CONV = 0,   // Conversion register
        REG_CONF = 1,   // Config register
        REG_LO_THR = 2, // Comparator low threshold
        REG_HI_THR = 3, // Comparator high threshold

		// Config bits
		CONF_OS = 1 << 15,
		CONF_MUX = 1 << 12, // 12:14
		CONF_PGA = 1 << 9,  // 9:11
		CONF_MODE = 1 << 8,
		CONF_DR = 1 << 5,   // 5:7
		CONF_COMP_MODE = 1 << 4,
		CONF_COMP_POL = 1 << 3,
		CONF_COMP_LAT = 1 << 2,
		CONF_COMP_QUE = 1   // 0:1
    };

    ADC(uint8_t addr)
        : Device(addr),
          _config(0) {
    }

    void probe() { Device::dummy_probe(); }

    // Pro forma
    void force_inline init() { }

    // Select input, gain, and SPS
    void config(Channel channel, FSR fsr, SPS sps) {
    	    _sps = sps;
        _config = ((channel | 0b100) << 12) | (fsr << 9) | (_sps << 5);
    }

    // Start one-shot read
    void start_single_read();

    // Wait for conversion done
    bool wait_conv();

    // Read conversion register
    uint16_t read_conv();

private:
    ADC(const ADC&);
    ADC& operator=(const ADC&);
};

}; // namespace ads1115

#endif  // _ADS1115_H_
