// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#include <stdint.h>
#include "common.h"
#include "ssd1963.h"
#include "task.h"
#include "panel/font/runes.h"
#include "hsd04319w1_a.h"

namespace rune_defs {
#include "panel/font/runes.inc"
}

namespace ssd1963 {

using namespace PanelInfo;

template <typename _DBPORT,
          typename _CTLPORT,
          uint8_t _CTL_CS,
          uint8_t _CTL_WR,
          uint8_t _CTL_RD,
          uint8_t _CTL_RS>
void Panel<_DBPORT, _CTLPORT, _CTL_CS, _CTL_WR, _CTL_RD, _CTL_RS>
          ::init() {

    DATA_PORT::P_DIR = 0xff;  // Output by default

    enum {
        XT_IN = 10000000ULL,    // 10MHz crystal
        LCD_A = 0b00000000,
        LCD_B = 0b00100000,
        LCD_G = 0b00000000,  // R,G,B order
        PIXEL_CLOCK = 1ULL * PXCLOCK_TYP,
        PLL_M = 40ULL,
        PLL_N = 5ULL
    };

    // Sync Timing Config Parameters
    enum {
        // Vertical config parameters
        PARM_VDP = VERT_VISIBLE - 1L,
        PARM_VT  = (VERT_FRONT_PORCH + VERT_VISIBLE + VERT_BACK_PORCH + VSYNC_WIDTH) - 1L,
        PARM_VPW = VSYNC_WIDTH - 1L,
        PARM_VPS = VSYNC_MOVE + VSYNC_WIDTH + VERT_BACK_PORCH,
        PARM_FPS = VSYNC_MOVE,

        // Horizontal config parameters
        PARM_HDP = HOR_VISIBLE - 1L,
        PARM_HT  = (HOR_FRONT_PORCH + HOR_VISIBLE + HSYNC_WIDTH + HOR_BACK_PORCH) - 1L,
        PARM_HPW = HSYNC_WIDTH - 1L,
        PARM_HPS = HSYNC_MOVE + HSYNC_WIDTH + HOR_BACK_PORCH,
        PARM_LPS = HSYNC_MOVE,
        PARM_LPSPP = HSYNC_SUBPIXEL_POS
    };

    enum {
        VCO_FREQ = XT_IN * PLL_M,
        ADJCLK = uint64_t(PIXEL_CLOCK) << 20ULL,

        PARM_LSHIFT = uint64_t(ADJCLK * PLL_N) / VCO_FREQ - 1,
        REFRESH_RATE = uint64_t(PIXEL_CLOCK) / (uint64_t(PARM_HT) * uint64_t(PARM_VT))
    };

    wcommand(CMD_EXIT_SLEEP_MODE);
    Task::wait(TIMER_MSEC(5));

    // Disable during init to avoid flickering
    wcommand(CMD_SET_DISPLAY_OFF);

    wcommand8(CMD_SET_PLL, 0);
    static const uint8_t pll[] = { PLL_M - 1, PLL_N - 1, 4 };
    wcommand_barr(CMD_SET_PLL_MN, 3, pll);

    wcommand8(CMD_SET_PLL, 0x01);   // Enable PLL
    Task::wait(TIMER_USEC(100));
    wcommand8(CMD_SET_PLL, 0x03);

    wcommand(CMD_SOFT_RESET);
    Task::sleep(TIMER_MSEC(5));

#define PARM16(P) ((P) >> 8) & 0xff, (P) & 0xff

    static const uint8_t sequence[] = {
       CMD_ENTER_NORMAL_MODE, 0,
       CMD_EXIT_INVERT_MODE, 0,
       CMD_SET_TEAR_OFF, 0,
       CMD_SET_ADDRESS_MODE, 1, 0x01,   // T-B, L-R etc (b1=flip H, b0=flip V)
       CMD_SET_PIXEL_FORMAT, 1, 0b01110000, // 24bpp
       CMD_SET_LCD_MODE, 7,
           LCD_A, LCD_B, PARM16(PARM_HDP), PARM16(PARM_VDP), LCD_G,
       CMD_SET_HORI_PERIOD, 8,
           PARM16(PARM_HT), PARM16(PARM_HPS), PARM_HPW, PARM16(PARM_LPS), PARM_LPSPP,
       CMD_SET_VERT_PERIOD, 7,
           PARM16(PARM_VT), PARM16(PARM_VPS), PARM_VPW, PARM16(PARM_FPS),
       CMD_SET_LSHIFT_FREQ, 3,
           (PARM_LSHIFT >> 16) & 0xff, PARM16(PARM_LSHIFT),
       CMD_SET_PIXEL_DATA_INTERFACE, 1, 0, // 8 bit interface (3x 8 bit per pixel)
       CMD_SET_POST_PROC, 4, 0x40, 0x80, 0x40,1,  // Contrast, Brigthness, Saturation
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
          uint8_t _CTL_RS>
void Panel<_DBPORT, _CTLPORT, _CTL_CS, _CTL_WR, _CTL_RD, _CTL_RS>
          ::clear() {
    set_rgb(0, 0, 0);
    fill(0, 0, HOR_VISIBLE, VERT_VISIBLE);
}

template <typename _DBPORT,
          typename _CTLPORT,
          uint8_t _CTL_CS,
          uint8_t _CTL_WR,
          uint8_t _CTL_RD,
          uint8_t _CTL_RS>
void Panel<_DBPORT, _CTLPORT, _CTL_CS, _CTL_WR, _CTL_RD, _CTL_RS>
          ::fill(uint16_t col, uint16_t row, uint16_t w, uint16_t h) {
    set_window(col, row, w, h);

    command_start(CMD_WRITE_MEMORY_START);

    for (uint16_t c = 0; c < w; ++c) {
        for (uint16_t r = 0; r < h; ++r) {
            data(_r);
            data(_g);
            data(_b);
        }
    }
    command_end();
    wcommand(CMD_NOP);
}

template <typename _DBPORT,
          typename _CTLPORT,
          uint8_t _CTL_CS,
          uint8_t _CTL_WR,
          uint8_t _CTL_RD,
          uint8_t _CTL_RS>
void Panel<_DBPORT, _CTLPORT, _CTL_CS, _CTL_WR, _CTL_RD, _CTL_RS>
          ::set_window(uint16_t col, uint16_t row, uint16_t w, uint16_t h) {

    command_start(CMD_SET_COLUMN_ADDRESS);
    data16(col);
    data16(col + w - 1);
    command_end();

    command_start(CMD_SET_PAGE_ADDRESS);
    data16(row);
    data16(row + h - 1);
    command_end();
}

template <typename _DBPORT,
          typename _CTLPORT,
          uint8_t _CTL_CS,
          uint8_t _CTL_WR,
          uint8_t _CTL_RD,
          uint8_t _CTL_RS>
void Panel<_DBPORT, _CTLPORT, _CTL_CS, _CTL_WR, _CTL_RD, _CTL_RS>
          ::render(uint16_t x, uint16_t y, Rune rune, uint16_t w, uint16_t h)  {
    set_window(x, y, w, h);

    // Run-length encoded... unpack.
    command_start(CMD_WRITE_MEMORY_START);

    // Format: 8 bits at a time, LSB first.
    // Top left to right, then move down one row from leftmost.  Bits are
    // packed into bytes and then run-length encoded.
    const uint8_t* start = rune_defs::rune_data + rune_defs::rune_offset[rune];

    for (const uint8_t *p = start; p < start + rune_defs::rune_size[rune];) {
        uint8_t n = *p++;
        if (!n) {
            for (int i = 0; i < 3*8; ++i) {
                data(0);
            }
            continue;
        }
        const uint8_t v = *p++;
        while (n--) {
            for (int i = 0; i < 8; ++i) {
                if (v & 1) {
                    data(_r); data(_g); data(_b);
                } else {
                    data(0); data(0); data(0);
                }
                v >>= 1;
            }
        }
    }
    command_end();
    wcommand(CMD_NOP);
}

};
