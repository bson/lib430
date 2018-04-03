// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef _24LC04_H_
#define _24LC04_H_

// 24LC04 and similar EEPROMs:
//   1 byte address with up to 3 bits of bank select in the control byte
//      (slave low addr bits).
//   16 byte page (not enforced)
// A0-A2 pins not used.
#include <stdint.h>
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
	bool write_bytes(uint16_t loc, const uint8_t* data, uint8_t len) {
		if (len == 0) {
			return true;
		}

		Device::mask_addr(0x7, loc >> 8);
		if (Device::start_write(loc)) {
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
			SysTimer::delay(TIMER_MSEC(MAX_WRITE_TIME+1));
		}
		if (len) {
			if (!write_bytes(loc, data, len)) {
				return false;
			}
			SysTimer::delay(TIMER_MSEC(MAX_WRITE_TIME+1));
		}
		return true;
	}

	// Read multiple pages, reading exactly len bytes.
	// XXX len must be 2 or more bytes
	bool read_pages(uint16_t loc, uint8_t *data, size_t len) {
		if (len == 0) {
			return true;
		}

		Device::mask_addr(0x7, loc >> 8);
		if (!Device::start_write(loc)) {
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

}; // namespace _24lc04

#endif // _24LC04_H_
