#ifndef _24LC04_H_
#define _24LC04_H_

// 24LC04 and similar EEPROMs:
//   1 byte address with up to 3 bits of bank select in the control byte
//      (slave low addr bits).
//   16 byte page (not enforced)
// A0-A2 pins not used.

#include "common.h"

// Initial underscore to avoid starting with digit
#define _24LC04_ADDR 0x50

// 'n' in namespace is to avoid starting with digit.
namespace n24lc04 {

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
		Device::mask_addr(0x7, loc >> 8);
		Device::transmit(loc, data);
	}

	// Write block of bytes. Up to 16 bytes within a 16-byte page.
	void write_bytes(uint16_t loc, const uint8_t* data, size_t len) {
		if (len == 1) {
			write(loc, *data);
			return;
		}

		Device::mask_addr(0x7, loc >> 8);
		if (Device::start_write(loc)) {
			while (len--) {
				Device::write(*data++);
			}
			Device::write_done();
		}
	}

	// Read single byte (random read).
	bool read(uint16_t loc, uint8_t* data) {
		Device::mask_addr(0x7, loc >> 8);
		Device::transmit(loc);
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

		Device::mask_addr(0x7, loc >> 8);
		Device::transmit(loc);
		if (Device::start_read(data++)) {
			while (n-- && Device::read(data++))
				;
			Device::read_done();
			len -= n;
			return true;
		}
		return false;
	}
};

}; // namespace _24lc04

#endif // _24LC04_H_
