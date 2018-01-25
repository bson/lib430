#ifndef _USBTMC_H_
#define _USBTMC_H_

#include "config.h"
#include "usb_dev.h"

// Simple USBTMC implementation, sitting on top of the simple USB device.
// Implements the usb488 subclass

template <typename Delegate>
class USBTMC {
    USB _usb;
public:
    USBTMC(const char* strs[], uint16_t nstrs, uint16_t plldiv);

    void init();
    void service();

private:
    USBTMC(const USBTMC&);
    USBTMC& operator=(const USBTMC&);
};


#endif // _USBTMC_H_
