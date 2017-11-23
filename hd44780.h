#ifndef _HD44780_H_
#define _HD44780_H_

namespace hd44780 {

template <typename Bus, typename Device, int LINEWIDTH=20>
class Display: public Device {
    uint8_t _bl;                // 0x80 if backlight is off, 0 = on

    enum { CMD_DELAY = 50,   // 50 usec delay after commands
           DATA_DELAY= 50    // 50 usec delay after data
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

    Display(Bus& bus, uint8_t addr) 
        : Device(bus, addr) {
    }
        
    void init(bool backlight);

    // Write value to controller register
    void write_reg(Register reg, uint8_t value);

    // Issue command (IR)
    void command(uint8_t cmd) {
        write_reg(RS_IR, cmd);
        //_timer.delay(USEC(CMD_DELAY));
    }

    void clear() {
        command(CLEARDISPLAY);
        _timer.delay(USEC(1500));
    }

    void home() {
        command(RETURNHOME);
        _timer.delay(USEC(1500));
    }

    void setpos(uint8_t l, uint8_t p) {
        const uint8_t addr[] = { 0, 64, LINEWIDTH, 64 + LINEWIDTH };
    
        command(CMD_SETDDRAMADDR | (addr[l] + p));
    }

    void putc(char c) {
        write(RS_DR, (uint8_t)c);
        //_timer.delay(USEC(LCD_DATA_DELAY));
    }

    void puts(const char* s) {
        while (*s)
            putc(*s++);
    }

private:
  	// Maps values to GPIO pins
    // Bit/pin      7   6   5   4   3   2   1   0
    // Signal       BL  D7  D6  D5  D4  E   RS  n.c.
    uint8_t bits(uint8_t d4, uint8_t e, uint8_t rs) const {
        return (_bl | ((d4) << 3) | ((e) << 2) | ((rs) << 1));
    }
private:
    Display(const Display&);
    Display& operator=(const Display&);
};

}; // namespace hd44780

#endif // _HD44780_H_
