//
// SSD1963 based panel
//

#ifndef _SSD1306_H_
#define _SSD1306_H_

#include "panel/font/runes.h"
#include "common.h"

namespace ssd1963 {

// DBPORT is an 8 bit data port
// CTLPORT is an 8 bit control port
// Operates in 8080 mode
template <typename _DBPORT,
          typename _CTLPORT,
          uint8_t _CTL_CS,
          uint8_t _CTL_WR,
          uint8_t _CTL_RD,
          uint8_t _CTL_RS,
          int _WIDTH  = 480,
          int _HEIGHT = 272>
class Panel: public Device {
public:
    typedef _DBPORT DATA_PORT;
    typedef _CTLPORT CONTROL_PORT;

    enum {
        WIDTH  = _WIDTH,
        HEIGHT = _HEIGHT,
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
        CMD_GET_PIXEL_DATA_INTERFACE = 0xf1,
    };

    void Panel() { }

    static void init();
    static void clear();
private:
    // A bunch of inlined primitives
    static void command_start(uint8_t cmd) {
        CONTROL_PORT.OUT |= MASK_RS | MASK_RD | MASK_WR;
        CONTROL_PORT.OUT &= ~MASK_WR;
        DATA_PORT.OUT = cmd;
        CONTROL_PORT.OUT |= MASK_WR;
    }
    static void command_end() { ; }

    static void data(uint8_t d) {
        CONTROL_PORT.OUT &= ~(MASK_WR | MASK_RS);
        DATA_PORT.OUT = d;
        CONTROL_PORT.OUT |= MASK_WR;
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
    static void wcommand16(uint8_t cmd, uint16_t param) {
        command_start(cmd);
        data16(param);
        command_end();
    }
    // Yet another variation: command followed by array of bytes
    static void wcommand_barr(uint8_t cmd, uint8_t nbytes, const uint8_t* data) {
        command_start(cmd);
        while (nbytes--)
            data(*data++);
        command_end();
    }

    static uint8_t rdata() {
        CONTROL_PORT.OUT &= ~MASK_RS;
        CONTROL_PORT.OUT &= ~MASK_RD;
        const uint8_t tmp = DATA_PORT.IN;
        CONTROL_PORT.OUT |= MASK_RD;
        return tmp;
    }
    static uint8_t rcommand(uint8_t cmd) {
        command_start(cmd);
        const uint8_t val = rdata();
        command_end();
        return tmp;
    }
};
}; // namespace ssd1963
