// 
// SSD1306 based 128x64 monochrome OLED on I2C panel
//
#include <stdint.h>

typedef unsigned int uint;

#include "ssd1306/ssd1306.h"
#include "ssd1306/font/runes.h"

#pragma CHECK_ULP("none")

// Realize code for I2C based device
#include "i2c_master/i2c.h"
template class ssd1306::Panel<I2CBus, I2CDevice>;

namespace rune_defs {
#include "ssd1306/font/runes.inc"
}

namespace ssd1306 {

template <typename Bus, typename Device>
void Panel<Bus,Device>::probe() {
    if (Device::state() != Device::UNATTACHED)
        return;

    Device::start_probe();
    bool ok = false;
    if (Device::start_write(CONTROL_CMD)) {
        Device::write(CMD_NO_OP);
        Device::write_done();
        ok = true;  // Responded to ACK, that's all we care about here
    }

    Device::end_probe(ok);

    // If attached, initialize and clear
    if (ok) {
        init();
        clear();
    }
}

template <typename Bus, typename Device>
void Panel<Bus,Device>::command(uint8_t cmd, uint param) {
    if (Device::start_write(CONTROL_CMD)) {
        Device::write(cmd);
        if (param != 0x100) {
            Device::write((uint8_t)param);
        }
        Device::write_done();
    }
}

template <typename Bus, typename Device>
void Panel<Bus,Device>::init() {
    // Initialize the display
    const uint8_t init_seq[] = {
        CONTROL_CMD, CMD_SET_CLOCK_OSC, CONTROL_CMD, 0x80, // Reset value
        CONTROL_CMD, CMD_SET_MULTIPLEX, CONTROL_CMD, PANEL_HEIGHT-1,
        CONTROL_CMD, CMD_SET_DISPLAY_OFFSET, CONTROL_CMD, 0, // Start at COM0
        CONTROL_CMD, CMD_SET_START_LINE+0,
        CONTROL_CMD, CMD_CHARGE_PUMP, CONTROL_CMD, 0x14,	// Use internal DC/DC converter
        CONTROL_CMD, CMD_COL_ADVANCE, CONTROL_CMD, 0,       // Horizontal (SSD1303 compat) mode
        CONTROL_CMD, CMD_SEGMENT_REMAP,
        CONTROL_CMD, CMD_COM_SCAN_DIRECTION1,
        CONTROL_CMD, CMD_SET_COM_PINS, CONTROL_CMD, 0x12,
        CONTROL_CMD, CMD_SET_CONTRAST, CONTROL_CMD, 0xcf,
        CONTROL_CMD, CMD_SET_PRECHARGE, CONTROL_CMD, 0xf1, // Use the internal DC/DC converter
        CONTROL_CMD, CMD_SET_VCOM_DETECT, CONTROL_CMD, 0x40,
        CONTROL_CMD, CMD_DISPLAY_ON_RESUME,
        CONTROL_CMD, CMD_SET_NORMAL_DISPLAY,
        CONTROL_CMD, CMD_STOP_SCROLL,
        CONTROL_CMD, CMD_DISPLAY_ON
    };

    Device::write_bytes(init_seq, sizeof init_seq);
}

template <typename Bus, typename Device>
bool Panel<Bus,Device>::output_col_byte(uint8_t byte) {
    if (!_running || _col >= _w) {
        if (_running) {
            Device::write_done();
        }

        command(CMD_SET_PAGE | ((_y/8) & 0xf));
        command(CMD_SET_LOW_COLUMN | (_x & 0xf));
        command(CMD_SET_HIGH_COLUMN | ((_x >> 4) & 0xf));
        _y -= 8;

        if (!Device::start_write(CONTROL_DATA)) {
            return false;
        }
        _running = true;
        _col = 0;
    }

    if (!Device::write(byte))
        return false;

    ++_col;
    return true;
}

template <typename Bus, typename Device>
void Panel<Bus,Device>::render(uint8_t x, uint8_t y, Rune rune, uint8_t w)  {
    _running = false;
    _x = x;
    _y = y;
    _w = w;
    _col = 0;

    // Run-length encoded... unpack.
    const uint8_t* start = rune_defs::rune_data + rune_defs::rune_offset[rune];
    for (const uint8_t *p = start; p < start + rune_defs::rune_size[rune];) {
        uint8_t n = *p++;
        if (!n) {
            if (!output_col_byte(0)) {
                return;
            }
            continue;
        }
        const uint8_t v = *p++;
        while (n--) {
            if (!output_col_byte(v)) {
                return;
            }
        }
    }

    if (_running) {
        Device::write_done();
    }
}

template <typename Bus, typename Device>
void Panel<Bus,Device>::clear() {
    enum { NUM_PAGES = PANEL_HEIGHT / 8 };

    for (uint8_t page = 0; page < NUM_PAGES; ++page) {
        command(CMD_SET_PAGE | (page & 0x7));
        command(CMD_SET_LOW_COLUMN | 0);
        command(CMD_SET_HIGH_COLUMN | 0);

        if (Device::start_write(CONTROL_DATA)) {
            for (uint i = PANEL_WIDTH; i > 0; --i) {
                if (!Device::write(0))
                    break;
            }
            Device::write_done();
        }
    }
}

}; // namespace ssd1306

#pragma RESET_ULP("all")
