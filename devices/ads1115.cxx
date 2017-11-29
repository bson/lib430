#ifdef _MAIN_

#include "common.h"
#include "ads1115.h"

namespace ads1115 {

template <typename Bus, typename Device>
void ADC<Bus,Device>::start_single_read() {
    if (Device::start_write(REG_CONF)) {
        Device::write((_config >> 8) | 1);  // Bit 8 => single shot conversion
        Device::write(_config & 0xff);
        Device::write_done();
    }
}


}; // namespace ads1115

#endif // _MAIN_
