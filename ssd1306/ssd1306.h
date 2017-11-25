/* 
 * SSD1306 based 128x64 monochrome OLED
 */

#ifndef _SSD1306_H_
#define	_SSD1306_H_

#include "common.h"
#include "ssd1306/font/runes.h"

namespace ssd1306 {

// Bus,Device need to implement the (simple) protocol of I2CBus, I2CDevice.
template <typename Bus, typename Device>
class Panel: public Device {
private:
    uint8_t _x, _y, _w, _col; // For columnizing
    bool _running;

public:
    enum {
        // There are a few different drivers out there, but they're
        // generally compatible, except the display dimensions vary:
        // 128x62, 128x64, 132x64, etc.  Adjust this to match the
        // panel.
        PANEL_WIDTH  = 132,
        PANEL_HEIGHT = 64
    };

    enum {
        CMD_DISPLAY_OFF           = 0xae,
        CMD_SET_CLOCK_OSC         = 0xd5,
        CMD_SET_MULTIPLEX         = 0xa8,
        CMD_SET_DISPLAY_OFFSET    = 0xd3,
        CMD_SET_START_LINE        = 0x40,
        CMD_CHARGE_PUMP           = 0x8d,
        CMD_SEGMENT_NO_REMAP      = 0xa0,
        CMD_SEGMENT_REMAP         = 0xa1,
        CMD_COM_SCAN_DIRECTION1   = 0xc0,
        CMD_COM_SCAN_DIRECTION2   = 0xc8,
        CMD_SET_COM_PINS          = 0xda,
        CMD_SET_BRIGHTNESS        = 0x82,
        CMD_SET_CONTRAST          = 0x81,
        CMD_SET_PRECHARGE         = 0xd9,
        CMD_SET_VCOM_DETECT       = 0xdb,
        CMD_DISPLAY_ON_RESUME     = 0xa4,
        CMD_SET_NORMAL_DISPLAY    = 0xa6,
        CMD_SET_INVERTED_DISPLAY  = 0xa7,
        CMD_DISPLAY_ON            = 0xaf,
        CMD_SET_LOW_COLUMN        = 0x00,
        CMD_SET_HIGH_COLUMN       = 0x10,
        CMD_SET_PAGE              = 0xb0,
        CMD_COL_ADVANCE           = 0x20,
        CMD_STOP_SCROLL           = 0x2e,
        CMD_NO_OP                 = 0xe3
    };

    enum {
        CONTROL_BYTE  = 0b11000000, // Single data byte
        CONTROL_CMD   = 0b10000000, // Single command byte
        CONTROL_DATA  = 0b01000000  // Continuous data stream
    };

public:
    Panel(Bus& bus, uint8_t addr)
        : Device(bus, addr) {
    }

    void probe();
    void init();
    void clear();
    void render(uint8_t x, uint8_t y, Rune rune, uint8_t w);

private:

    void command(uint8_t cmd, uint param = 0x100);

    bool output_col_byte(uint8_t byte);

    // Disabled
    Panel(const Panel&);
    Panel& operator=(const Panel&);
};

}; // namespace ssd1306

#endif	// _SSD1306_H_
