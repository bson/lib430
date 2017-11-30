/*
 * A note on timing: a lot of the HD44780 command and data timings
 * don't apply since simply the time to change the GPIO pins for the
 * next write over I2C exceeds those.  However, the baseline data and
 * command timing has been left in but commented out.  Longer timings
 * on power-on configuration and the slower commands are left in since
 * they're greater than I2C bus latencies.  The timings were verified
 * on a DSO by capturing E+RS.  
 *
 * The timing should resolve itself with future work to the Timer.
 * 
 */
#ifdef _MAIN_

#include "common.h"
#include "cpu/g2553.h"
#include "timer.h"
#include "hd44780.h"

namespace hd44780 {

#define Disp Display<Bus,Device,LINEWIDTH>

// Device init
template <typename Bus, typename Device, int LINEWIDTH>
void Disp::init(bool backlight) {
    _bl   = 0x80; /* backlight ? 0x80 : 0; */
    
    Device::init();

    _sysTimer.delay(TIMER_MSEC(15));
    write_reg(IR, 0x3);
    _sysTimer.delay(TIMER_MSEC(5));
    write_reg(IR, 0x3);
    _sysTimer.delay(TIMER_USEC(1000));
    write_reg(IR, 0x2);
    _sysTimer.delay(TIMER_USEC(CMD_DELAY));

    command(CMD_FUNCTIONSET | MODE_4BIT | LINES_2 | DOTS_5X8);
    command(CMD_DISPLAYCONTROL | DISPLAYON | CURSOROFF | BLINKOFF);
    command(CMD_ENTRYMODESET | ENTRYINC);

    clear();
    home();
}

// Write to register
template <typename Bus, typename Device, int LINEWIDTH>
void Disp::write_reg(Disp::Register reg, uint8_t value) {

    const uint8_t hi = value >> 4;
    const uint8_t lo = value & 0xf;

    if (Device::start_write(bits(hi, 1, reg))) { // E = hi
        Device::write(bits(hi, 0, reg)); // E = lo
        Device::write(bits(lo, 1, reg)); // E = hi
        Device::write(bits(lo, 0, reg)); // E = lo
        Device::write_done();
    }
}

};  // namespace hd44780

#endif // _MAIN_

