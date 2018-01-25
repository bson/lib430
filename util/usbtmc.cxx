#ifdef _MAIN_

#include "config.h"
#include "usbtmc.h"
#include "usb_dev.h"
#include "common.h"

// Needs to be supplied by application
#ifdef ENABLE_DEBUG
#define DMSG xprintf
extern void xprintf(const char *fmt, ...);
#else
#define DMSG(...)
#define dumpsetup() 0
#endif

static const USB::DeviceDescriptor usbdev _ro_ = {
        sizeof(USB::DeviceDescriptor),  // 8
        USB::TYPE_DEVICE,
        0x0200,       // USB 2.0.0
        0, 0, 0,      // Class, subclass, protocol defined in interface
        64,           // Max packet size for EP0
        USB_VID, USB_PID, // VID, PID
        0x0100,      // Device version 1.0.0
        1, 2, 3,     // Mfg, prod, sn strings
        1            // 1 configuration
};

static const USB::ConfigDescriptor usbconf _ro_ = {
        sizeof(USB::ConfigDescriptor), // 8
        USB::TYPE_CONFIG,
        0,         // Total length (filled in when sent)
        1,         // 1 = # of interfaces
        1,         // 1 = config value for to select this config
        0,         // Config string
        1 << 6,    // Self powered
        20         // Will draw max 50mA
};

static const USB::InterfaceDescriptor usbif _ro_ = {
        sizeof(USB::InterfaceDescriptor), // 8
        USB::TYPE_INTERFACE,
        0,           // 0 = first interface
        0,           // no alt config
        2,           // two endpoints (interrupt IN, OUT)
        0xfe,3,1,    // USBTMC (app) class, subclass, protocol (usb488)
        0,           // Interface string
};

static const USB::EndpointDescriptor usbep[] _ro_ = {
    {
     // EP1 OUT - bulk
     sizeof(USB::EndpointDescriptor), USB::TYPE_ENDPOINT,
     USB::EP_ADDR_IN | 1, USB::EP_ATTR_TTBULK, 512, 0
    },
    {
     // EP1 IN - bulk
     sizeof(USB::EndpointDescriptor), USB::TYPE_ENDPOINT,
     USB::EP_ADDR_OUT | 1, USB::EP_ATTR_TTBULK, 512, 0
    },
    {
     // EP2 IN - interrupt, poll 50ms
     sizeof(USB::EndpointDescriptor), USB::TYPE_ENDPOINT,
     USB::EP_ADDR_IN | 2, USB::EP_ATTR_TTINTR, 64, 50,
    }
};

template <typename Delegate>
USBTMC<Delegate>::USBTMC(const char* strs[], uint16_t nstrs,
               uint16_t plldiv)
    : _usb(&usbdev, &usbconf, &usbif,
           usbep, NELEM(usbep),
           strs, nstrs,
           plldiv)
{
    ;
}

template <typename Delegate>
void USBTMC<Delegate>::init() {
    _usb.init();
    _usb.add_endpoint(1, 64, 64);
    _usb.add_endpoint(2, 64, 64);
    _usb.enable();
}

#ifdef DEBUG_TRACE
static void dumpsetup() {
    const USB::SetupRequest* req = _usb.get_setup();

    DMSG("  SETUP REQ: T=%x R=%x V=%x I=%x L=%x\n",
            req->type, req->request, req->value, req->index, req->length);
}
#endif

template <typename Delegate>
void USBTMC<Delegate>::service() {
    uint16_t event;

    while ((event = _usb.pending_event()) != USB::EVENT_NONE) {
        switch (event) {
        case USB::EVENT_CONNECT:
            Delegate::connected(_usb.connected());
            DMSG("USB: connect/disconnect event %d\n", _usb.connected());
            continue;

        case USB::EVENT_SUSPEND:
            if (!_usb.suspended())
                _usb.start();  // Start up PLL

            DMSG("USB: suspend/resume event\n");
            continue;

        case USB::EVENT_CONFIG:
            DMSG("USB: host configure event\n");
            continue;

        case USB::EVENT_SETUP:
            DMSG("USB: setup\n");
            dumpsetup();
            continue;

        case USB::EVENT_STALL:
            DMSG("USB: stalled\n");
            continue;

        case USB::EVENT_SETADDR:
            DMSG("USB: set addr %d\n", _usb.addr());
            continue;

        case USB::EVENT_SETUPHK:
            DMSG("USB: application control/setup hook\n");
            dumpsetup();
            continue;

        case USB::EVENT_EP1_OUT:
            DMSG("USB: EP1 OUT\n");
            continue;

        case USB::EVENT_EP1_IN:
            DMSG("USB: EP1 IN\n");
            continue;

        default:
            DMSG("USB: other event: %x\n", event);
            continue;
        }
    }
}

#endif // _MAIN_
