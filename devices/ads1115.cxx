#ifdef _MAIN_

#include "common.h"
#include "ads1115.h"

namespace ads1115 {

template <typename Bus, typename Device>
void ADC<Bus,Device>::start_single_read() {
    if (Device::start_write(REG_CONF)) {
        Device::write((_config >> 8) | 0x81);  // Bit 8 => single shot conversion
        Device::write(_config & 0xff);
        Device::write_done();
    }
}

template <typename Bus, typename Device>
bool ADC<Bus,Device>::wait_conv() {
	static const uint8_t to[] = {1000/8, 1000/16, 1000/32, 1000/64, 1000/128,
								1000/250, 1000/475, 1000/7};
	uint8_t data_hi;
	uint8_t data_lo;

	Device::transmit(REG_CONF);
	const SysTimer::Future deadline = _sysTimer.future(TIMER_MSEC(to[_sps]));
	bool ok = true;
	if (Device::start_read(&data_hi) && Device::read(&data_lo)) {
		while ((data_hi & (CONF_OS >> 8)) && !_sysTimer.due(deadline)) {
			_sysTimer.delay(TIMER_USEC(500));
			if (!Device::read(&data_hi) || !Device::read(&data_lo)) {
				ok = false;
				break;
			}
		}
		Device::read_done();
		return ok && !(data_hi & (CONF_OS >> 8));
	}
	return false;
}

template <typename Bus, typename Device>
uint16_t ADC<Bus,Device>::read_conv() {
	uint8_t data_hi;
	uint8_t data_lo;

	Device::transmit(REG_CONV);
	if (Device::start_read(&data_hi) && Device::read(&data_lo)) {
		Device::read_done();
	}

	return (data_hi << 8) | data_lo;
}


}; // namespace ads1115

#endif // _MAIN_
