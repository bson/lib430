// 
// SSD1306 based 128x64 monochrome OLED on I2C panel
//
#include <stdint.h>

typedef unsigned int uint;

#include "i2c_master/i2c.h"
#include "ssd1306/ssd1306.h"
#include "ssd1306/font/runes.h"

namespace rune_defs {
#include "ssd1306/font/runes.inc"
}

void Ssd1306::probe() {
    if (I2CDevice::state() != I2CDevice::UNATTACHED)
        return;

    I2CDevice::start_probe();
    bool ok = false;
    if (I2CDevice::start_write(CONTROL_CMD)) {
        I2CDevice::write(CMD_NO_OP);
        I2CDevice::write_done();
        ok = true;  // Responded to ACK, that's all we care about here
    }

    I2CDevice::end_probe(ok);

    // If attached, initialize and clear
    if (ok) {
        init();
        clear();
    }
}

void Ssd1306::command(uint8_t cmd, uint param) {
    if (I2CDevice::start_write(CONTROL_CMD)) {
        I2CDevice::write(cmd);
        if (param != 0x100) {
            I2CDevice::write((uint8_t)param);
        }
        I2CDevice::write_done();
    }
}

void Ssd1306::init() {
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

    I2CDevice::write_bytes(init_seq, sizeof init_seq);
}

bool Ssd1306::output_col_byte(uint8_t byte) {
    if (!_running || _col >= _w) {
        if (_running) {
            I2CDevice::write_done();
        }

        command(CMD_SET_PAGE | ((_y/8) & 0xf));
        command(CMD_SET_LOW_COLUMN | (_x & 0xf));
        command(CMD_SET_HIGH_COLUMN | ((_x >> 4) & 0xf));
        _y -= 8;

        if (!I2CDevice::start_write(CONTROL_DATA)) {
            return false;
        }
        _running = true;
        _col = 0;
    }

    if (!I2CDevice::write(byte))
        return false;

    ++_col;
    return true;
}

void Ssd1306::render(uint8_t x, uint8_t y, Rune rune, uint8_t w)  {
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
        I2CDevice::write_done();
    }
}

void Ssd1306::clear() {
    enum { NUM_PAGES = PANEL_HEIGHT / 8 };

    for (uint8_t page = 0; page < NUM_PAGES; ++page) {
        command(CMD_SET_PAGE | (page & 0x7));
        command(CMD_SET_LOW_COLUMN | 0);
        command(CMD_SET_HIGH_COLUMN | 0);

        if (I2CDevice::start_write(CONTROL_DATA)) {
            for (uint i = PANEL_WIDTH; i > 0; --i) {
                if (!I2CDevice::write(0))
                    break;
            }
            I2CDevice::write_done();
        }
    }
}
