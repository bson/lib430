#ifndef _24LC256_H_
#define _24LC256_H_

// 24LC256 and similar EEPROMs:
//   2 byte memory address with low slave addr bits from pins A0-A2
//   64 byte page (not enforced)

#include "common.h"

// Initial underscore to avoid starting with digit
#define _24LC256_ADDR(A0,A1,A2) \
	(0x50 | (A0) | ((A1) << 1) | ((A2) << 2))

// 'n' in namespace is to avoid starting with digit.
namespace n24lc256 {

template <typename Bus, typename Device>
class Eeprom: public Device {
public:
	enum { PAGESIZE = 64 };

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
	void write_bytes(uint16_t loc, const uint8_t* data, uint8_t len) {
		if (len == 1) {
			write(loc, *data);
			return;
		}

		if (Device::start_write(loc >> 8)) {
			Device::write(loc);

			while (len--) {
				Device::write(*data++);
			}
			Device::write_done();
		}
	}

	// Write a large block, potentially greater than a page.  Loc must be on
	// a page boundary.
	void write_pages(uint16_t loc, const uint8_t* data, size_t len) {
		while (len > PAGESIZE) {
			write_bytes(loc, data, PAGESIZE);
			data += PAGESIZE;
			len -= PAGESIZE;
		}
		if (len) {
			write_bytes(loc, data, len & 0xff);
		}
	}

	// Read single byte (random read).
	bool read(uint16_t loc, uint8_t* data) {
		Device::transmit(loc >> 8, loc);
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

		Device::transmit(loc >> 8, loc);
		if (Device::start_read(data++)) {
			while (n-- && Device::read(data++))
				;
			Device::read_done();
			len -= n;
			return true;
		}
		return false;
	}

	// Read multiple pages, reading exactly len bytes.
	bool read_pages(uint16_t loc, uint8_t *data, size_t len) {
		while (len > PAGESIZE) {
			size_t nread = PAGESIZE;
			if (!read_bytes(loc, data, nread) || nread != PAGESIZE)
				return false;
			len -= nread;
		}
		if (len) {
			size_t nread = len;
			if (!read_bytes(loc, data, nread) || nread != len)
				return false;
		}
		return true;
	}
};

}; // namespace _24lc256

#endif // _24LC256_H_
