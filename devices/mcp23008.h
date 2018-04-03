// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef _MCP23008_H_
#define _MCP23008_H_

#include "common.h"

#define MCP23008_ADDR(A0,A1,A2)  \
	(0x20 | (A0) | ((A1) << 1) | ((A2) << 2))

namespace mcp23008 {

template <typename Device>
class Expander: public Device {
public:
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
           MCP23008_OLAT   = 0x0A
    };

    Expander(uint8_t addr)
        : Device(addr) {
    }

    void probe() {
        if (Device::state() == Device::UNATTACHED) {
            Device::dummy_probe();
            init();
        }
    }

    void init();

    // Implement write sequence so that bytes written appear on the
    // GPIO pin outputs.
    bool start_write(uint8_t data) {
        return Device::start_write(MCP23008_GPIO) && Device::write(data);
    }
                         
    bool write(uint8_t data) { return Device::write(data); }
    void write_done() { Device::write_done(); }

    // Put one or two bytes on the GPIO outputs.
    bool transmit(uint8_t byte1, uint16_t byte2 = 0x100) {
        bool ok = false;
        if (start_write(byte2)) {
            ok = (byte2 == 0x100) || write(byte2);
            write_done();
        }
        return ok;
    }
    
    void set(uint8_t data) { write(data); }
private:
    Expander(const Expander&);
    Expander& operator=(const Expander&);
};

};

#endif // _MCP23008_H_
