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

template <typename Device>
class Eeprom: public Device {
public:
	enum {
		PAGESIZE = 64,
		MAX_WRITE_TIME = 5  // in msec
	};

	Eeprom(uint8_t addr)
		: Device(addr) {
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
	bool write_bytes(uint16_t loc, const uint8_t* data, uint8_t len) {
		if (len == 1) {
			write(loc, *data);
			return;
		}

		if (Device::start_write(loc >> 8)) {
			Device::write(loc);

			while (len--) {
				if (!Device::write(*data++)) {
					return false;
				}
			}
			Device::write_done();
			return true;
		}
		return false;

	}

	// Write a large block, potentially greater than a page.  Loc must be on
	// a page boundary.  Inserts a delay between writes.
	bool write_pages(uint16_t loc, const uint8_t* data, size_t len) {
		while (len >= PAGESIZE) {
			if (!write_bytes(loc, data, PAGESIZE)) {
				return false;
			}
			loc += PAGESIZE;
			data += PAGESIZE;
			len -= PAGESIZE;
			// XXX handle this instead by spending up to MAX_WRITE_TIME trying
			// to read the page back, verifying it wrote correctly.  The read
			// will fail until the write has finished.
			_sysTimer.delay(TIMER_MSEC(MAX_WRITE_TIME+1));
		}
		if (len) {
			if (!write_bytes(loc, data, len)) {
				return false;
			}
			_sysTimer.delay(TIMER_MSEC(MAX_WRITE_TIME+1));
		}
		return true;
	}

	// Read multiple pages, reading exactly len bytes.
	// XXX len must be 2 or more bytes
	bool read_pages(uint16_t loc, uint8_t *data, size_t len) {
		if (len == 0) {
			return true;
		}

		if (!Device::start_write(loc >> 8) || !Device::write(loc)) {
			return false;
		}
		if (Device::restart_read(data++)) {
			--len;
			while (len > 1) {
				--len;
				if (!Device::read(data++))
					return false;
			}
		    Device::read_end(data++);
		    --len;
			return len == 0;
		}
		return false;
	}
};

}; // namespace _24lc256

#endif // _24LC256_H_
