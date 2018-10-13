// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifdef _MAIN_

#include "common.h"
#include "mcp23008.h"

namespace mcp23008 {

template <typename Device>
void Expander<Device>::init() {
    // MCP23008 - set all pins to output, no pull-up, no interrupts
    Device::transmit(REG_IODIR, 0);
    Device::transmit(REG_IPOL, 0);
    Device::transmit(REG_GPPU, 0);
    Device::transmit(REG_GPINTEN, 0);
    Device::transmit(REG_IOCON, 0x20);  // Set SEQOP
}

};

#endif // _MAIN_
