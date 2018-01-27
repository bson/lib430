#ifdef _MAIN_

#include "config.h"
#include "usbtmc.h"
#include "usb_dev.h"
#include "common.h"

// Needs to be supplied by application
#ifdef DEBUG_TRACE
#define DMSG xprintf
extern void xprintf(const char *fmt, ...);
#else
#define DMSG(...)
#define dumpsetup() 0
#endif

static const USB::DeviceDescriptor usbdev = {
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

static const USB::ConfigDescriptor usbconf = {
        sizeof(USB::ConfigDescriptor), // 8
        USB::TYPE_CONFIG,
        0,         // Total length (filled in when sent)
        1,         // 1 = # of interfaces
        1,         // 1 = config value for to select this config
        0,         // Config string
        1 << 6,    // Self powered
        20         // Will draw max 50mA
};

static const USB::InterfaceDescriptor usbif = {
        sizeof(USB::InterfaceDescriptor), // 8
        USB::TYPE_INTERFACE,
        0,           // 0 = first interface
        0,           // no alt config
        2,           // two endpoints (interrupt IN, OUT)
        0xfe,3,1,    // USBTMC (app) class, subclass, protocol (usb488)
        0,           // Interface string
};

static const USB::EndpointDescriptor usbep[] = {
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
     // EP2 IN - interrupt, poll 50ms.  Size 2 is mandated by USB488
     sizeof(USB::EndpointDescriptor), USB::TYPE_ENDPOINT,
     USB::EP_ADDR_IN | 2, USB::EP_ATTR_TTINTR, 0x02, 50,
    }
};

static USBTMC_Capabilities cap = {
  0, 0,        // Status, reserved
  0x0100,      // Version, 1.0.0
  0b00000100,  // Interface cap: D2=USB488.2, no REN_CONTROL/LOCKOUT, no TRIGGER
  0b00001000,  // Device cap: D3=SCPI, D2=SR1 capable (intr-IN EP), RL0, DL0
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  // Reserved
};

static const char* strs[3];

template <typename Delegate>
USBTMC<Delegate>::USBTMC(const char* manuf,
                         const char* prod,
                         const char* serial,
                         uint16_t plldiv) {
    strs[0] = manuf;
    strs[1] = prod;
    strs[2] = serial;

    USB::configure(&usbdev, &usbconf, &usbif,
                   usbep, NELEM(usbep),
                   strs, NELEM(strs),
                   plldiv);
}

template <typename Delegate>
void USBTMC<Delegate>::init() {
    USB::init();
    USB::add_endpoint(1, 64, 64);
    USB::add_endpoint(2, 64, 64);
    USB::enable();
}

#ifdef DEBUG_TRACE
static void dumpsetup() {
    const USB::SetupRequest* req = USB::get_setup();

    DMSG("  SETUP REQ: T=%x R=%x V=%x I=%x L=%x\n",
            req->type, req->request, req->value, req->index, req->length);
}
#endif

template <typename Delegate>
void USBTMC<Delegate>::control_req(const USB::SetupRequest* setup) {
    switch (setup->request) {
    case GET_CAPABILITIES:
        if (setup->value != 0) {
            cap.status = STATUS_FAILED;
        } else {
            cap.status = STATUS_SUCCESS;
        }
        USB::write(0, &cap, sizeof cap);
        break;

    case INITIATE_ABORT_BULK_IN:
    case INITIATE_ABORT_BULK_OUT: {
        // We don't current have a mechanism for long bulk ops, so just fail them
        const uint8_t tag = setup->value;
        const uint16_t response = (tag << 8) | STATUS_FAILED;
        USB::write(0, &response, 2);
        break;
    }
    case INITIATE_CLEAR: {
        const uint8_t response = STATUS_FAILED;
        USB::write(0, &response, 1);
        break;
    }
    case READ_STATUS_BYTE: {
        const uint16_t tag = setup->value & 0x7f;
        const uint32_t response = (tag << 8) | STATUS_SUCCESS;
        USB::write(2, &response, 3);
        break;
    }
    case CHECK_ABORT_BULK_OUT_STATUS:
    case CHECK_ABORT_BULK_IN_STATUS:
    case CHECK_CLEAR_STATUS:
        DMSG("Unimplemented USBTMC class control request %x\n", setup->request);
        USB::stall(0);
        break;

    case INDICATOR_PULSE:
        Delegate::pulse();
        break;
    default:
        USB::stall(0);
        break;
    }
}

template <typename Delegate>
void USBTMC<Delegate>::bulk_dev_req() {
    if (_bulk_out_len < sizeof(BulkBase)) {
        USB::stall(1);
        return;
    }

    const BulkBase* base = (const BulkBase*)_bulk_out_req;
    if (base->tag != ~base->taginv & 0xff) {
        DMSG("Tag error: %x,%x\n", base->tag, base->taginv);
        USB::stall(1);
        return;
    }
    // Both bulk-out commands are the same size
    if (_bulk_out_len < sizeof(DevDepBulk)) {
         USB::stall(1);
         return;
     }

    switch (base->msgid) {
    case DEV_DEP_MSG_OUT: {
        DevDepBulk* msg = (DevDepBulk*)_bulk_out_req;
        if (!(msg->attrs & ATTR_EOM) || msg->size == 0
                || msg->size > 64 - 1 - sizeof(DevDepBulk)) {
            USB::stall(1);
            break;
        }
        msg->data[msg->size] = 0; // 0 terminate for easy C use
        Delegate::query((char*)msg->data, msg->size);
        break;
    }
    case REQUEST_DEV_DEP_MSG_IN:{
        DevDepBulk* msg = (DevDepBulk*)_bulk_out_req;
        if (msg->size == 0
                || msg->size > 64 - 1 - sizeof(DevDepBulk)) {
            USB::stall(1);
            break;
        }
        msg->data[msg->size] = 0; // 0 terminate for easy C use
        Delegate::ask((char*)msg->data, msg->size);
        break;
    }
    default:
        USB::stall(1);
        break;
    }
}

template <typename Delegate>
void USBTMC<Delegate>::reply(const uint8_t* data, int len) {
    static DevDepBulk r = { { 0,0,0,0 }, 0, 0, 0, 0, 0, 0 };
    memcpy(&r.base, _bulk_out_req, sizeof r.base);
    r.size = len + 1;
    r.attrs = ATTR_EOM;
    memcpy(r.data, data, len);
    r.data[len] = '\n';   // USB488
    USB::write(1, &r, sizeof r + len);
}

template <typename Delegate>
void USBTMC<Delegate>::srq() {
    const uint16_t status = 0x4081;  // 0x40 = RQS set, see IEEE-488.2
    USB::write(2, &status, 2);
}

template <typename Delegate>
void USBTMC<Delegate>::service() {
    uint16_t event;

    while ((event = USB::pending_event()) != USB::EVENT_NONE) {
        switch (event) {
        case USB::EVENT_CONNECT:
            Delegate::connected(USB::connected());
            DMSG("USB: connect/disconnect event %d\n", USB::connected());
            continue;

        case USB::EVENT_SUSPEND:
            if (!USB::suspended())
                USB::start();  // Start up PLL

            DMSG("USB: suspend/resume event %d\n", USB::suspended());
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
            DMSG("USB: set addr %d\n", USB::addr());
            continue;

        case USB::EVENT_SETUPHK: {
            DMSG("USB: application control/setup hook\n");
            dumpsetup();

            const USB::SetupRequest* setup = USB::get_setup();
            if (((setup->type >> 5) & 3) == 1) {
                control_req(setup);
            }
            continue;
        }
        case USB::EVENT_EP1_OUT:
            DMSG("USB: EP1 OUT\n");
            USB::read(1, _bulk_out_req, _bulk_out_len);
            bulk_dev_req();
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
