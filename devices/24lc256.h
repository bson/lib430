#ifndef _24LC256_H_
#define _24LC256_H_

// 24LC256 and similar EEPROMs:
//   2 byte memory address with low slave addr bits from pins A0-A2
//   64 byte page (not enforced)

#include "common.h"

#define _24LC256_ADDR(A0,A1,A2) \
	(0xa0 | (A0) | ((A1) << 1) | ((A2) << 2))

namespace n24lc256 {

template <typename Bus, typename Device>
class Eeprom: public Device {
public:
	Eeprom(Bus& bus, uint8_t addr)
		: Device(bus, addr) {
	}

	void force_inline probe() { Device::dummy_probe(); }
	void force_inline init() { }

	// Write single byte
	void write(uint16_t loc, uint8_t data) {
		if (Device::start_write(loc >> 8)) {
			Device::write(loc);
			Device::write(data);
		}
	}

	// Write block of bytes. Up to 64 bytes within a 64-byte page.
	void write_bytes(uint16_t loc, const uint8_t* data, size_t len) {
		if (--len == 0) {
			write(loc, *data);
			return;
		}

		if (Device::start_write(loc >> 8)) {
			Device::write(loc);

			while (--len > 0) {
				Device::write(*data++);
			}
			Device::write_done();
		}
	}

	// Read single byte (random read).
	bool read(uint16_t loc, uint8_t* data) {
=		Device::transmit(loc >> 8, loc);
		if (Device::start_read(data)) {
			Device::read_done();
			return true;
		}
		return false;
	}

	// Read block of bytes.  len is the desired length and will be updated
	// with length actually read.  Returns true on success.
	// Random read.
	bool read_bytes(uint16_t loc, uint8_t *data, size_t& len) {
		size_t n = len;
		if (--n == 0) {
			if (read(loc, data)) {
				len = 1;
				return true;
			}
			return false;
		}

=		Device::transmit(loc >> 8, loc);
		if (Device::start_read(data++)) {
			while (--n > 0 && Device::read(data++))
				;
			Device::read_done();
			len -= n;
			return true;
		}
		return false;
	}
};

}; // namespace _24lc256

#endif // _24LC256_H_
