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

template <typename Device>
class Eeprom: public Device {
public:
	enum {
		PAGESIZE = 16,
		MAX_WRITE_TIME = 5    // in msec
	};

	Eeprom(uint8_t addr)
		: Device(addr) {
	}

	void force_inline probe() { Device::dummy_probe(); }
	void force_inline init() { }

	// Write single byte
	void write(uint16_t loc, uint8_t data) {
		Device::mask_addr(0x7, loc >> 8);
		Device::transmit(loc, data);
	}

	// Write block of bytes. Up to 16 bytes within a 16-byte page.  Note that
	// this doesn't wait for the write to finish.
	void write_bytes(uint16_t loc, const uint8_t* data, uint8_t len) {
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

	// Write a large block, potentially greater than a page.  Loc must be on
	// a page boundary.  Inserts a delay between writes.
	void write_pages(uint16_t loc, const uint8_t* data, size_t len) {
		while (len > PAGESIZE) {
			write_bytes(loc, data, PAGESIZE);
			data += PAGESIZE;
			len -= PAGESIZE;
			// XXX handle this instead by spending up to MAX_WRITE_TIME trying
			// to read the page back, verifying it wrote correctly.  The read
			// will fail until the write has finished.
			if (len) {
				_sysTimer.delay(TIMER_MSEC(MAX_WRITE_TIME));
			}
		}
		if (len) {
			write_bytes(loc, data, len & 0xff);
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

	// Read multiple pages, reading exactly len bytes.
	bool read_pages(uint16_t loc, uint8_t *data, size_t len) {
		while (len > PAGESIZE) {
			size_t nread = PAGESIZE;
			if (!read_bytes(loc, data, nread) || nread != PAGESIZE)
				return false;
			len -= PAGESIZE;
		}
		if (len) {
			size_t nread = len;
			if (!read_bytes(loc, data, nread) || nread != len)
				return false;
		}
		return true;
	}
};

}; // namespace _24lc04

#endif // _24LC04_H_
