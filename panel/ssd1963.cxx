#include <stdint.h>
#include "common.h"
#include "ssd1963.h"
#include "systimer.h"


namespace ssd1963 {

template <typename _DBPORT,
          typename _CTLPORT,
          uint8_t _CTL_CS,
          uint8_t _CTL_WR,
          uint8_t _CTL_RD,
          uint8_t _CTL_RS,
          int _WIDTH,
          int _HEIGHT>
void Panel<_DBPORT, _CTLPORT, _CTL_CS, _CTL_WR, _CTL_RD, _CTL_RS, _WIDTH, _HEIGHT>
          ::init() {
    // XXX automate this based on dimensions, refresh, and porch constants
    enum {
        LCD_A = 0b00000000,
        LCD_B = 0b00100000,
        LCD_HPS = WIDTH-1,
        LCD_VPS = HEIGHT-1,
        LCD_G = 0b00000000,  // R,G,B order
        PIXEL_CLOCK = 4000000ULL, // 4MHz
        PLL_CLOCK = 113333333ULL, // PLL @ 113.33MHz
        CLOCK_DIV = uint32_t((PIXEL_CLOCK << 20ULL) / PLL_CLOCK)
    };

    wcommand(CMD_EXIT_SLEEP_MODE);
    _sysTimer.delay(TIMER_MSEC(500));

    // 10MHz * 34/3 = 113.33MHz
    wcommand_barr(CMD_SET_PLL_MN, 3, (const uint8_t*)"\x31\x02\x04");

    wcommand8(CMD_SET_PLL, 0x01);   // Enable PLL
    _sysTimer.delay(TIMER_USEC(100));
    wcommand8(CMD_SET_PLL, 0x03);   // Enable and use PLL

    static const uint8_t sequence[] = {
       CMD_ENTER_NORMAL_MODE, 0,
       CMD_EXIT_INVERT_MODE, 0,
       CMD_SET_TEAR_OFF, 0,
       CMD_SET_ADDRESS_MODE, 1, 0x00,   // T-B, L-R etc (b1=flip H, b0=flip V)
       CMD_SET_PIXEL_FORMAT, 1, 0b01100000, // 18bpp
       CMD_SET_LCD_MODE, 7,
           LCD_A, LCD_B, LCD_HPS >> 8, LCD_HPS & 0xff, LCD_VPS >> 8, LCD_VPS & 0xff, LCD_G,
       CMD_SET_HORI_PERIOD, 8,
           LCD_HPS >> 8, LCD_HPS & 0xff, LCD_HPS >> 8, LCD_HPS & 0xff, 7, 0, 0, 0,
       CMD_SET_VERT_PERIOD, 7,
           LCD_VPS >> 8, LCD_VPS & 0xff, 0, 4, 1, 0, 0,
       CMD_SET_POST_PROC, 4, 0x40, 0x80, 0x40,  // Contrast, Brigthness, Saturation
       CMD_SET_LSHIFT_FREQ, 3,
           (CLOCK_DIV >> 16) & 0xff, (CLOCK_DIV >> 8) & 0xff, CLOCK_DIV & 0xff,
       CMD_SET_PIXEL_DATA_INTERFACE, 1, 0, // 8 bit interface (3x 6 bit)
       CMD_SET_DISPLAY_ON, 0
    };

    for (const uint8_t* p = sequence; p < sequence + sizeof sequence; ) {
        const uint8_t cmd = *p++;
        const uint8_t n   = *p++;
        if (n) {
            wcommand_barr(cmd, n, p);
            p += n;
        } else {
            wcommand(cmd);
        }
    }
}

template <typename _DBPORT,
          typename _CTLPORT,
          uint8_t _CTL_CS,
          uint8_t _CTL_WR,
          uint8_t _CTL_RD,
          uint8_t _CTL_RS,
          int _WIDTH,
          int _HEIGHT>
void Panel<_DBPORT, _CTLPORT, _CTL_CS, _CTL_WR, _CTL_RD, _CTL_RS, _WIDTH, _HEIGHT>
          ::clear() {
    set_rgb(0, 0, 0);
    fill(0, 0, WIDTH, HEIGHT);
}

template <typename _DBPORT,
          typename _CTLPORT,
          uint8_t _CTL_CS,
          uint8_t _CTL_WR,
          uint8_t _CTL_RD,
          uint8_t _CTL_RS,
          int _WIDTH,
          int _HEIGHT>
void Panel<_DBPORT, _CTLPORT, _CTL_CS, _CTL_WR, _CTL_RD, _CTL_RS, _WIDTH, _HEIGHT>
          ::fill(uint16_t col, uint16_t row, uint16_t w, uint16_t h) {
    command_start(CMD_SET_PAGE_ADDRESS);
    data(row >> 8);
    data(row);
    data((row + h - 1) >> 8);
    data(row + h - 1);
    command_end();

    command_start(CMD_SET_COLUMN_ADDRESS);
    data(col >> 8);
    data(col);
    data((col + w - 1) >> 8);
    data(col + w - 1);
    command_end();

    command_start(CMD_WRITE_MEMORY_START);
    for (uint16_t r = 0; r < w; ++r)
        for (uint16_t c = 0; c < h; ++c) {
            data(_r);
            data(_g);
            data(_b);
        }
    command_end();
}


};
