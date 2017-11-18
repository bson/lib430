/* 
 * File:   ada_lcd.h

 * Simple interface to Adafruit's I2C LCD backpack and a 20x4 display.  The 'pack has
 * an MCP23008 I2C GPIO expander driving a standard HD44780 header.  It can only 
 * write to the LCD controller, not read.  Since it has only 8 GPIO pins it has to drive
 * the display in 4-bit mode, meaning each register write requires two cycles.
 * 
 * This driver can be compiled for 16-char LCDs, and will work for 1/2 line models
 * also.  Just don't try to set a position outside the display.
 * 
 * A note on timing: a lot of the HD44780 command and data timings don't apply
 * since simply the time to change the GPIO pins for the next write over I2C exceeds
 * those.  However, the baseline data and command timing has been left in but commented
 * out.  Longer timings on power-on configuration and the slower commands are left
 * in since they're greater than I2C bus latencies.  The timings were verified on
 * a DSO by capturing E+RS.
 * 
 * With a 100kHz I2C bus this code will output almost 700 cps, which means a line
 * of 20 outputs in under 30ms.
 * 
 * A proper production implementation should probably employ an interrupt-driven state
 * machine layered on top of a similarly structured I2C driver.
 *
 */

#ifndef _ADA_LCD_H_
#define	_ADA_LCD_H_

#include <msp430.h>
#include "i2c_master/i2c.h"
#include "timer.h"


// Maps values to GPIO pins
// Bit/pin      7   6   5   4   3   2   1   0
// Signal       BL  D7  D6  D5  D4  E   RS  n.c.
#define GPBITS(D4,E,RS)  (_bl | ((D4) << 3) | ((E) << 2) | ((RS) << 1))

class AdaLcd: public I2CDevice {
    uint8_t _bl;                // 0x80 if backlight is off, 0 = on

    enum { LCD_LINEWIDTH = 20,   // 20 char display
           LCD_CMD_DELAY = 50,   // 50 usec delay after commands
           LCD_DATA_DELAY= 50    // 50 usec delay after data
    };

    // Most of these were lifted from Adafruit's Arduino library on github
    enum { MCP23008_IODIR  = 0x00,
           MCP23008_IPOL   = 0x01,
           MCP23008_GPINTEN= 0x02,
           MCP23008_DEFVAL = 0x03,
           MCP23008_INTCON = 0x04,
           MCP23008_IOCON  = 0x05,
           MCP23008_GPPU   = 0x06,
           MCP23008_INTF   = 0x07,
           MCP23008_INTCAP = 0x08,
           MCP23008_GPIO   = 0x09,
           MCP23008_OLAT   = 0x0A,
       
           LCD_RS_IR       = 0,
           LCD_RS_DR       = 1,
       
           // command
           LCD_CLEARDISPLAY= 0x01,
           LCD_RETURNHOME  = 0x02,
           LCD_ENTRYMODESET= 0x04,
           LCD_DISPLAYCONTROL=0x08,
           LCD_CURSORSHIFT = 0x10,
           LCD_FUNCTIONSET = 0x20,
           LCD_SETCGRAMADDR= 0x40,
           LCD_SETDDRAMADDR= 0x80,
           // bits for display entry mode
           LCD_ENTRYDEC    = 0x00,
           LCD_ENTRYINC    = 0x02,
           LCD_ENTRYSHIFT  = 0x01,
           // bits for display on/off control
           LCD_DISPLAYON   = 0x04,
           LCD_DISPLAYOFF  = 0x00,
           LCD_CURSORON    = 0x02,
           LCD_CURSOROFF   = 0x00,
           LCD_BLINKON     = 0x01,
           LCD_BLINKOFF    = 0x00,
           // bits for display/cursor shift
           LCD_DISPLAYMOVE = 0x08,
           LCD_CURSORMOVE  = 0x00,
           LCD_MOVERIGHT   = 0x04,
           LCD_MOVELEFT    = 0x00,
           // flags for function set
           LCD_8BITMODE    = 0x10,
           LCD_4BITMODE    = 0x00,
           LCD_2LINE       = 0x08,
           LCD_1LINE       = 0x00,
           LCD_5x10DOTS    = 0x04,
           LCD_5x8DOTS     = 0x00
    };

public:
    AdaLcd(I2CBus& bus, uint8_t addr)
        : I2CDevice(bus, addr) {
    }

    // Device init
    void init(bool bl) {
        _bl   = bl ? 0x80 : 0;
    
        // MCP23008 - set all pins to output, no pull-up, no interrupts
        I2CDevice::transmit(MCP23008_IODIR, 0);
        I2CDevice::transmit(MCP23008_GPPU, 0);
        I2CDevice::transmit(MCP23008_GPINTEN, 0);
    
        _timer.delay(MSEC(15));
        write(LCD_RS_IR, 0x3);
        _timer.delay(MSEC(5));
        write(LCD_RS_IR, 0x3);
        _timer.delay(USEC(1000));
        write(LCD_RS_IR, 0x2);
        _timer.delay(USEC(LCD_CMD_DELAY));

        command(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS);
        command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);
        command(LCD_ENTRYMODESET | LCD_ENTRYINC);

        clear();
        home();
    }

    // Write IR
    // reg is 2 for DR, 0 for IR
    void write(uint8_t reg, uint8_t value) {
        static uint8_t prev_reg = 255;
        const uint8_t hi = value >> 4;

        if (reg != prev_reg) {
            I2CDevice::transmit(MCP23008_GPIO, GPBITS(hi, 0, reg));    // So new RS is valid on rising E
            prev_reg = reg;
        }
        I2CDevice::transmit(MCP23008_GPIO, GPBITS(hi, 1, reg)); // E = hi
        I2CDevice::transmit(MCP23008_GPIO, GPBITS(hi, 0, reg)); // E = lo

        const uint8_t lo = value & 0xf;

        I2CDevice::transmit(MCP23008_GPIO, GPBITS(lo, 1, reg)); // E = hi
        I2CDevice::transmit(MCP23008_GPIO, GPBITS(lo, 0, reg)); // E = lo
    }


    void command(uint8_t cmd) {
        write(LCD_RS_IR, cmd);
        //_timer.delay(USEC(LCD_CMD_DELAY));
    }

    void clear() {
        command(LCD_CLEARDISPLAY);
        _timer.delay(USEC(1500));
    }

    void home() {
        command(LCD_RETURNHOME);
        _timer.delay(USEC(1500));
    }

    void setpos(uint8_t l, uint8_t p) {
        const uint8_t addr[] = { 0, 64, LCD_LINEWIDTH, 64 + LCD_LINEWIDTH };
    
        command(LCD_SETDDRAMADDR | (addr[l] + p));
    }


    void putc(char c) {
        write(LCD_RS_DR, (uint8_t)c);
        //_timer.delay(USEC(LCD_DATA_DELAY));
    }

    void puts(const char* s) {
        while (*s)
            putc(*s++);
    }
};

#endif	// _ADA_LCD_H_


