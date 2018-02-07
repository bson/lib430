#ifndef _HD44780_H_
#define _HD44780_H_

#include "common.h"

namespace hd44780 {

template <typename Device, int LINEWIDTH=20>
class Display: public Device {
    uint8_t _bl;                // 0x80 if backlight is off, 0 = on

    enum { CMD_DELAY = 1000,  // delay after commands in usec
           DATA_DELAY= 50    // delay after data in usec
    };

    enum Register {
        IR = 0,
        DR = 1,
        NO_REG = 255
    };

public:
    enum { // Commands
           CMD_CLEARDISPLAY= 0x01,
           CMD_RETURNHOME  = 0x02,
           CMD_ENTRYMODESET= 0x04,
           CMD_DISPLAYCONTROL=0x08,
           CMD_CURSORSHIFT = 0x10,
           CMD_FUNCTIONSET = 0x20,
           CMD_SETCGRAMADDR= 0x40,
           CMD_SETDDRAMADDR= 0x80,

           // bits for display entry mode
           ENTRYDEC    = 0x00,
           ENTRYINC    = 0x02,
           ENTRYSHIFT  = 0x01,
           // bits for display on/off control
           DISPLAYON   = 0x04,
           DISPLAYOFF  = 0x00,
           CURSORON    = 0x02,
           CURSOROFF   = 0x00,
           BLINKON     = 0x01,
           BLINKOFF    = 0x00,
           // bits for display/cursor shift
           DISPLAYMOVE = 0x08,
           CURSORMOVE  = 0x00,
           MOVERIGHT   = 0x04,
           MOVELEFT    = 0x00,
           // flags for function set
           MODE_8BIT   = 0x10,
           MODE_4BIT   = 0x00,
           LINES_2     = 0x08,
           LINES_1     = 0x00,
           DOTS_5X10   = 0x04,
           DOTS_5X8    = 0x00
    };

    Display(uint8_t addr)
        : Device(addr) {
    }
        
    void init(bool backlight);

    // Device probe
    void probe() {
    		if (Device::state() == Device::UNATTACHED) {
    			Device::probe();
        		init(_bl);
    		}
    }

    // Write value to controller register
    void write_reg(Register reg, uint8_t value);

    // Issue command (IR)
    void command(uint8_t cmd) {
        write_reg(IR, cmd);
        SysTimer::delay(TIMER_USEC(CMD_DELAY));
    }

    void clear() {
        command(CMD_CLEARDISPLAY);
        SysTimer::delay(TIMER_USEC(1500));
    }

    void home() {
        command(CMD_RETURNHOME);
        // SysTimer::delay(TIMER_USEC(1500));
    }

    void setpos(uint8_t l, uint8_t p) {
        const uint8_t addr[] = { 0, 64, LINEWIDTH, 64 + LINEWIDTH };
    
        command(CMD_SETDDRAMADDR | (addr[l] + p));
    }

    // Enable/disable cursor
    void cursor(bool state) {
    		if (state) {
    			command(CMD_DISPLAYCONTROL | DISPLAYON | CURSORON | BLINKOFF);
    		} else {
    			command(CMD_DISPLAYCONTROL | DISPLAYON | CURSOROFF | BLINKOFF);
    		}
    }

    void force_inline putc(char c) {
        write_reg(DR, (uint8_t)c);
        //SysTimer::delay(TIMER_USEC(DATA_DELAY));
    }

    void puts(const char* s) {
        while (*s)
            putc(*s++);
    }

private:
  	// Maps values to GPIO pins
    // Bit/pin      7   6   5   4   3   2   1   0
    // Signal       BL  D7  D6  D5  D4  E   RS  RW=0
    uint8_t force_inline bits(uint8_t nybble, uint8_t e, Register rs) const {
        return _bl | ((nybble & 0xf) << 3) | (e << 2) | (uint8_t(rs) << 1);
    }
private:
    Display(const Display&);
    Display& operator=(const Display&);
};

}; // namespace hd44780

#endif // _HD44780_H_
