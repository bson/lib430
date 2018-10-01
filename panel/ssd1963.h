// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

//
// SSD1963 based panel
//

#ifndef _SSD1306_H_
#define _SSD1306_H_

#include "font/runes.h"
#include "../common.h"

namespace ssd1963 {

// DBPORT is an 8 bit data port
// CTLPORT is an 8 bit control port
// Operates in 8080 mode
template <typename _DBPORT,
          typename _CTLPORT,
          uint8_t _CTL_CS,
          uint8_t _CTL_WR,
          uint8_t _CTL_RD,
          uint8_t _CTL_RS>
class Panel {
    static uint8_t _r;
    static uint8_t _g;
    static uint8_t _b;

public:
    typedef _DBPORT DATA_PORT;
    typedef _CTLPORT CONTROL_PORT;

    enum {
        MASK_CS = _CTL_CS,
        MASK_WR = _CTL_WR,
        MASK_RD = _CTL_RD,
        MASK_RS = _CTL_RS
    };

    enum {
        CMD_NOP                = 0,
        CMD_SOFT_RESET         = 0x01,
        CMD_GET_POWER_MODE     = 0x0a,
        CMD_GET_ADDRESS_MODE   = 0x0b,
        CMD_GET_PIXEL_FORMAT   = 0x0c,
        CMD_GET_DISPLAY_MODE   = 0x0d,
        CMD_GET_SIGNAL_MODE    = 0x0e,
        CMD_ENTER_SLEEP_NODE   = 0x10,
        CMD_EXIT_SLEEP_MODE    = 0x11,
        CMD_ENTER_PARTIAL_MODE = 0x12,
        CMD_ENTER_NORMAL_MODE  = 0x13,
        CMD_EXIT_INVERT_MODE   = 0x20,
        CMD_ENTER_INVERT_MODE  = 0x21,
        CMD_SET_GAMMA_CURVE    = 0x26,
        CMD_SET_DISPLAY_OFF    = 0x28,
        CMD_SET_DISPLAY_ON     = 0x29,
        CMD_SET_COLUMN_ADDRESS = 0x2a,
        CMD_SET_PAGE_ADDRESS   = 0x2b,
        CMD_WRITE_MEMORY_START = 0x2c,
        CMD_READ_MEMORY_START  = 0x2e,
        CMD_SET_PARTIAL_AREA   = 0x30,
        CMD_SET_SCROLL_AREA    = 0x33,
        CMD_SET_TEAR_OFF       = 0x34,
        CMD_SET_TEAR_ON        = 0x35,
        CMD_SET_ADDRESS_MODE   = 0x36,
        CMD_SET_SCROLL_MODE    = 0x37,
        CMD_EXIT_IDLE_MODE     = 0x38,
        CMD_ENTER_IDLE_MODE    = 0x39,
        CMD_SET_PIXEL_FORMAT   = 0x3a,
        CMD_WRITE_MEMORY_CONTINUE = 0x3c,
        CMD_READ_MEMORY_CONTINUE = 0x3e,
        CMD_SET_TEAR_SCANLINE  = 0x44,
        CMD_GET_SCANLINE       = 0x45,
        CMD_READ_DDB           = 0xa1,
        CMD_SET_LCD_MODE       = 0xb0,
        CMD_GET_LCD_MODE       = 0xb1,
        CMD_SET_HORI_PERIOD    = 0xb4,
        CMD_GET_HORI_PERIOD    = 0xb5,
        CMD_SET_VERT_PERIOD    = 0xb6,
        CMD_GET_VERT_PERIOD    = 0xb7,
        CMD_SET_GPIO_CONF      = 0xb8,
        CMD_GET_GPIO_CONF      = 0xb9,
        CMD_SET_GPIO_VALUE     = 0xba,
        CMD_GET_GPIO_STATUS    = 0xbb,
        CMD_SET_POST_PROC      = 0xbc,
        CMD_GET_POST_PROC      = 0xbd,
        CMD_SET_PWM_CONF       = 0xbe,
        CMD_GET_PWM_CONF       = 0xbf,
        CMD_SET_LCD_GEN0       = 0xc0,
        CMD_GET_LCD_GEN0       = 0xc1,
        CMD_SET_LCD_GEN1       = 0xc2,
        CMD_GET_LCD_GEN1       = 0xc3,
        CMD_SET_LCD_GEN2       = 0xc4,
        CMD_GET_LCD_GEN2       = 0xc5,
        CMD_SET_LCD_GEN3       = 0xc6,
        CMD_GET_LCD_GEN3       = 0xc7,
        CMD_SET_GPIO0_ROP      = 0xc8,
        CMD_GET_GPIO0_ROP      = 0xc9,
        CMD_SET_GPIO1_ROP      = 0xca,
        CMD_GET_GPIO1_ROP      = 0xcb,
        CMD_SET_GPIO2_ROP      = 0xcc,
        CMD_GET_GPIO2_ROP      = 0xcd,
        CMD_SET_GPIO3_ROP      = 0xce,
        CMD_GET_GPIO3_ROP      = 0xcf,
        CMD_SET_DBC_CONF       = 0xd0,
        CMD_GET_DBC_CONF       = 0xd1,
        CMD_SET_DBC_TH         = 0xd4,
        CMD_GET_DBC_TH         = 0xd5,
        CMD_SET_PLL            = 0xe0,
        CMD_SET_PLL_MN         = 0xe2,
        CMD_GET_PLL_MN         = 0xe3,
        CMD_GET_PLL_STATUS     = 0xe4,
        CMD_SET_DEEP_SLEEP     = 0xe5,
        CMD_SET_LSHIFT_FREQ    = 0xe6,
        CMD_GET_LSHIFT_FREQ    = 0xe7,
        CMD_SET_PIXEL_DATA_INTERFACE = 0xf0,
        CMD_GET_PIXEL_DATA_INTERFACE = 0xf1
    };

    Panel() { }

    static void init();
    static void clear();
    static void fill(uint16_t col, uint16_t row, uint16_t w, uint16_t h);
    static void set_rgb(uint8_t r, uint8_t g, uint8_t b) {
        _r = r; _g = g; _b = b;
    }
    static void render(uint16_t x, uint16_t y, Rune rune, uint16_t w, uint16_t h);

    // Set PWM brightness, only usable for modules that use the PWM
    static void set_brightness(uint8_t b) {
        command_start(CMD_SET_PWM_CONF);
        data(0x0e);  // 300Hz @ 120MHz PLL
        data(b);
        data(0x01);  // Enable
        data(0);
        data(0);
        data(0);
        command_end();
    }

    static void adjust_cbs(uint8_t cont  = 0x40,
                           uint8_t brite = 0x80,
                           uint8_t sat   = 0x40) {
        uint8_t v[3] = { cont, brite, sat, 1 };
        wcommand_barr(CMD_SET_POST_PROC, 4, v);
    }

private:
    // Set window
    static void set_window(uint16_t col, uint16_t row, uint16_t w, uint16_t h);

    // A bunch of inlined primitives
    static void command_start(uint8_t cmd) {
        CONTROL_PORT::P_OUT |= MASK_RS | MASK_RD | MASK_WR | MASK_CS;
        CONTROL_PORT::P_OUT &= ~(MASK_CS | MASK_RS);  // CS active across command
        DATA_PORT::P_OUT = cmd;     // Data on bus
        CONTROL_PORT::P_OUT &= ~MASK_WR;  // WR active; RS = 0 = command
        CONTROL_PORT::P_OUT |= MASK_WR; // WR inactive - data is latched on this edge
        CONTROL_PORT::P_OUT |= MASK_RS; // Restore RS
    }
    static void command_end() {
        CONTROL_PORT::P_OUT |= MASK_CS | MASK_RD | MASK_WR | MASK_RS;  // CS inactive
    }

    // In a series of data writes CS is kept low across all of them, then
    // released by command_end().
    static void data(uint8_t d) {
        CONTROL_PORT::P_OUT |= MASK_RS | MASK_WR | MASK_RD;   // RS = 1 = data
        DATA_PORT::P_OUT = d;             // Data on bus
        CONTROL_PORT::P_OUT &= ~MASK_WR;  // WR active; RS = 1 = data
        CONTROL_PORT::P_OUT |= MASK_WR;   // WR inactive - data is latched on this edge
    }
    static void data16(uint16_t d) {
        data(d >> 8);
        data(d);
    }
    static void wcommand(uint8_t cmd) {
        command_start(cmd);
        command_end();
    }
    static void wcommand8(uint8_t cmd, uint8_t param) {
        command_start(cmd);
        data(param);
        command_end();
    }
    // Yet another variation: command followed by array of bytes
    static void wcommand_barr(uint8_t cmd, uint8_t nbytes, const uint8_t* d) {
        command_start(cmd);
        while (nbytes--)
            data(*d++);

        command_end();
    }

    static uint8_t rdata() {
        DATA_PORT::P_DIR = 0x00;  // Input
        CONTROL_PORT::P_OUT |= MASK_RS;  // Data
        CONTROL_PORT::P_OUT &= ~MASK_RD;
        const uint8_t tmp = DATA_PORT::P_IN;
        CONTROL_PORT::P_OUT |= MASK_RD;
        DATA_PORT::P_DIR = 0xff;  // Return to output
        return tmp;
    }
    static uint8_t rcommand(uint8_t cmd) {
        command_start(cmd);
        const uint8_t val = rdata();
        command_end();
        return val;
    }
};
}; // namespace ssd1963

#endif // _SSD1963_H_

