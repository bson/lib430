#ifdef _MAIN_

#include "common.h"
#include "ads1115.h"

namespace ads1115 {

template <typename Device>
void ADC<Device>::start_single_conv() {
    if (Device::start_write(REG_CONF)) {
        Device::write((_config >> 8) | 0x81);  // Bit 8 => single shot conversion
        Device::write(_config & 0xff);
        Device::write_done();
    }
}

template <typename Device>
bool ADC<Device>::wait_conv() {
	static const uint8_t to[] = {1000/8, 1000/16, 1000/32, 1000/64, 1000/128,
								1000/250, 1000/475, 1000/860};
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

template <typename Device>
uint16_t ADC<Device>::read_conv() {
	uint8_t data_hi;
	uint8_t data_lo;

	Device::transmit(REG_CONV);
	if (Device::start_read(&data_hi) && Device::read(&data_lo)) {
		Device::read_done();
	}

	return (data_hi << 8) | data_lo;
}

template <typename Device>
uint32_t ADC<Device>::read_cal() {
	const uint16_t reading = read_conv();
	if (!_cal_table_hi || !_cal_table_hi) {
		return uint32_t(reading) << 16;
	}
	if (reading == 0x7fff) {
		return ~0UL;
	}

	uint32_t result = 0;
	const uint32_t *cal_table = reading >= HI_CAL ? _cal_table_hi : _cal_table_lo;
	const uint32_t *table = cal_table + 1;

	for (uint16_t mask = 1U << 15; mask; mask >>= 1) {
		if (reading & mask) {
			result += *table;
		}
		++table;
	}
	result -= cal_table[0];

	return result;
}

}; // namespace ads1115

#endif // _MAIN_
