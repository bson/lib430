// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef _PCF8574_H_
#define _PCF8574_H_

#include "common.h"

// PCF8574 and PCF8574A differ only in their address.
#define PCF8574_ADDR(A0,A1,A2) \
	(0x20 | (A0) | ((A1) << 1) | ((A2) << 2))

#define PCF8574A_ADDR(A0,A1,A2) \
	(0x38 | (A0) | ((A1) << 1) | ((A2) << 2))

namespace pcf8574 {

// The PCF8574 is a simple device where anything written to it
// shows up on the outputs, so it can transparently pass through
// all writes.
template <typename Device>
class Expander: public Device {
public:
    Expander(uint8_t addr)
        : Device(addr) {
    }

    void probe() { Device::dummy_probe(); }

    void init() { }

    void set(uint8_t bits) { Device::transmit(bits); }
private:
    Expander(const Expander&);
    Expander& operator=(const Expander&);
};

};

#endif // _PCF8574_H_
