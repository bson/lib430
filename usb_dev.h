#ifndef _USB_DEV_H_
#define _USB_DEV_H_

// Very simple USB device manager
//  Single device
//  Single configuration
//  One interface per endpoint pair
//  One in+out endpoint pair per interface
//  English language strings only


#include "common.h"
#include "config.h"
#include "systimer.h"
#include "cpu/cpu.h"

#ifdef __MSP430_HAS_USB__

class USB {
public:
    // Setup request data
    struct SetupRequest {
        uint8_t type;
        uint8_t request;
        uint16_t value;
        uint16_t index;
        uint16_t length;
    };

    // Requests
    enum {
        REQ_GET_STATUS = 0,
        REQ_CLEAR_FEATURE = 1,
        REQ_SET_FEATURE = 3,
        REQ_SET_ADDRESS = 5,
        REQ_GET_DESC = 6,
        REQ_SET_DESC = 7,
        REQ_GET_CONF = 8,
        REQ_SET_CONF = 9
    };

    // Request recipients
    enum {
        REQ_DEVICE = 0,
        REQ_INTERFACE,
        REQ_ENDPOINT,
        REQ_OTHER
    };

    // Descriptor types
    enum {
        TYPE_DEVICE = 1,
        TYPE_CONFIG = 2,
        TYPE_STRING = 3,
        TYPE_INTERFACE = 4,
        TYPE_ENDPOINT = 5
    };

    // Descriptors.  These are passed in verbatim  to the constructor,
    // to facilitate storing them preconstructed in ROM.

    // Device descriptor
    struct DeviceDescriptor {
        uint8_t  length;    // sizeof(DeviceDescriptor)
        uint8_t  type;      // TYPE_DEVICE
        uint16_t bcdusb;    // 0x0200 = 2.0 spec version in BCD
        uint8_t  devclass;  // Device class (USB org assigned)
        uint8_t  subclass;  // Sub class (USB org assigned)
        uint8_t  protocol;  // Protocol (USB org assigned)
        uint8_t  pktsize;    // 64
        uint16_t vid;
        uint16_t pid;
        uint16_t devrel;    // Device release, BCD
        uint8_t  manuf;     // Manufacturer string descriptor
        uint8_t  prod;      // Product string descriptor
        uint8_t  serial;    // Serial number string descriptor
        uint8_t  numconf;   // 1 = number of configurations
    };

    // Configuration descriptor
    struct ConfigDescriptor {
        uint8_t  length;    // sizeof(ConfigDescriptor)
        uint8_t  type;      // TYPE_CONFIG
        uint16_t total;     // total length including interfaces and enpoints
        uint8_t  numif;     // # of interfaces
        uint8_t  confval;   // 1 = config value
        uint8_t  confstr;   // Config string descriptor
        uint8_t  attrs;     // Attribute bitmap
        uint8_t  maxpower;  // Max current draw, times 2mA
    };

    // Interface descriptor
    struct InterfaceDescriptor {
        uint8_t  length;    // sizeof(InterfaceDescriptor)
        uint8_t  type;      // TYPE_INTERFACE
        uint8_t  ifnum;     // interface number (zero based)
        uint8_t  alt;       // alternative setting (not used, zero)
        uint8_t  numep;     // Number of endpoints
        uint8_t  ifclass;   // Interface class (USB org assigned)
        uint8_t  ifsubclass; // (USB org assigned)
        uint8_t  ifproto;    // (USB org assigned)
        uint8_t  ifstring;   // Interface string
    };

    // Endpoint descriptor
    struct EndpointDescriptor {
        uint8_t  length;     // sizeof(EndpointDescriptor)
        uint8_t  type;       // TYPE_ENDPOINT
        uint8_t  addr;       // Address (bit 7 = 0 for out, 1 for in)
        uint8_t  attr;       // Attribute bits
        uint16_t maxpkt;     // Max packet size
        uint8_t  interval;   // Polling interval in ms (2.0) or 0.125ms (2.0 HS)
    };

    // Endpoint attributes
    enum {
        // Transfer type
        EP_ATTR_TTCONTROL = 0b00,
        EP_ATTR_TTISO     = 0b01,  // Isochronous
        EP_ATTR_TTBULK    = 0b10,  // Bulk
        EP_ATTR_TTINTR    = 0b11,  // Interrupt

        // Isochronous sync
        EP_ATTR_ISONOSYNC = 0b0000,
        EP_ATTR_ISOASYNC  = 0b0100,
        EP_ATTR_ISOADAPT  = 0b1000,
        EP_ATTR_ISOSYNC   = 0b1100,

        // Isochronous usage mode
        EP_ATTR_ISO_DATAEP = 0b000000, // Data EP
        EP_ATTR_ISO_FBEP   = 0b010000, // Feedback EP
        EP_ATTR_ISO_DFBEP  = 0b100000  // Data feedback EP
    };

    // Endpoint direction (for addr)
    enum {
        EP_ADDR_IN  = 0,
        EP_ADDR_OUT = 0x80
    };

    // String descriptor (minus actual string)
    struct StringDescriptor {
        uint8_t  length;     // Length of descriptor
        uint8_t  type;       // 3 = string
    };

    // String desc 0 (english language)
    struct StringDesc0 {
        uint8_t  length;     // sizeof(StringDesc0)
        uint8_t  type;       // 3 = string
        uint16_t lang;       // 0x409 = US english
    };

    // Events.  Lower values have higher priority.
    enum {
        EVENT_NONE     = 0,

        EVENT_CONNECT  = 1,     // Connect/disconnect
        EVENT_SUSPEND  = 2,     // Suspend/resume event
        EVENT_RESET    = 4,
        EVENT_CONFIG   = 8,     // Host configure
        EVENT_SETUP    = 0x10,  // Setup processed
        EVENT_SETUPHK  = 0x20,  // Unhandled setup in buffer
        EVENT_STALL    = 0x40,  // Stalled
        EVENT_SETADDR  = 0x80,  // Received set address

        // Data receive events
        EVENT_EP1_OUT = 0x100,
        EVENT_EP2_OUT = 0x200,
        EVENT_EP3_OUT = 0x400,
        EVENT_EPx_OUT = 0x800,  // Any other EP, 4-7 OUT
        EVENT_EP1_IN  = 0x1000,
        EVENT_EP2_IN  = 0x2000,
        EVENT_EP3_IN  = 0x4000,
        EVENT_EPx_IN  = 0x8000,  // Any other EP, 4-7 IN
    };

private:
    const DeviceDescriptor* _dev_desc;
    const ConfigDescriptor* _conf_desc;
    const InterfaceDescriptor* _if_desc;
    const EndpointDescriptor* _ep_descs;
    const uint8_t _nep_descs;  // # of EP descriptors
    const char** _strings;
    const uint8_t _nstrings;

    const uint16_t _plldiv;

    volatile uint16_t _events;  // Event mask

    volatile bool _connected;
    volatile bool _connect_change;
    volatile bool _suspended;
    volatile bool _suspend_change;

    uintptr_t _brk;
    uint8_t _neps;     // Number of endpoint pairs
    uint8_t _configured;
    uint8_t _addr;     // Bus address 0-127

public:
    USB(const DeviceDescriptor* dev,
        const ConfigDescriptor* conf,
        const InterfaceDescriptor* if_,
        const EndpointDescriptor* eps,
        uint8_t neps,
        const char** strs,
        uint8_t nstrs,
        uint16_t plldiv)
        : _dev_desc(dev),
          _conf_desc(conf),
          _if_desc(if_),
          _ep_descs(eps),
          _nep_descs(neps),
          _strings(strs),
          _nstrings(nstrs),
          _plldiv(plldiv),
          _connected(false),
          _connect_change(false),
          _suspended(false),
          _suspend_change(false),
          _brk((uintptr_t)&USBSTABUFF),
          _neps(0),
          _configured(0),
          _events(0)
    {
        ;
    }

    // Get next event, or 0 if none; removes event reuturned from pending set.
    uint16_t pending_event() {
       if (!_events)
            return 0;

       NoInterrupt g;

        const uint16_t pending = _events & ~(_events - 1);
        _events &= ~pending;
        return pending;
    }

    // Post an event.  Interrupts need to be disabled.
    void post_event(uint16_t event) {
        _events |= event;
    }

    // Initialize
    void init();
    void start() {
        enable_pll();
    }

    // Enable an endpoint and allocate buffer for it
    void add_endpoint(int n, uint16_t rxbuf_size, uint16_t txbuf_size);

    // Enable operation
    void enable() { USBCTL |= FEN; }

    // Write data to endpoint
    void write(int n, const void* data, int len);

    // Read data from endpoint
    void read(int n, void* data, int& len);

    // Respond with stall on endpoint N
    void stall(int n);

    // For application to handle SETUP
    const SetupRequest* get_setup() {
        return (const SetupRequest*)&USBSUBLK;
    }

    // Simply ack endpoint N with a 0 byte data stage
    void ack(int n) { write(n, NULL, 0); }

    // State inquiries
    bool connected() const { return _connected; }
    bool suspended() const { return _suspended; }
    bool configured() const { return _configured; }
    uint8_t addr() const { return _addr; }

// private:
    // These are called from an interrupt context.  They're public because TI's compiler is broken
    // and can't have an interrupt handler be a static class method.
    void connect_isr() {
        _connected = true;
        _suspended = false;
        post_event(EVENT_CONNECT);
        post_event(EVENT_SUSPEND);
    }

    void disconnect_isr() {
        _connected = false;
        post_event(EVENT_CONNECT);
        UnlockConf u;
        USBPLLCTL &= ~UPLLEN;
    }

    void input_isr(uint16_t endpoint);
    void output_isr(uint16_t endpoint);
    void setup_isr();
    void device_req_isr(const SetupRequest* setup);
    void interface_req_isr(const SetupRequest* setup);
    void endpoint_req_isr(const SetupRequest* setup);

    void suspend_isr() {
        _suspended = true;
        post_event(EVENT_SUSPEND);
        UnlockConf u;
        USBPLLCTL &= ~UPLLEN;
    }
    void resume_isr() {
        _suspended = false;
        post_event(EVENT_SUSPEND);
    }
    void reset_isr() {
        post_event(EVENT_RESET);
    }

private:
    class UnlockConf {
    public:
        UnlockConf() { USBKEYPID = USBKEY; }
        ~UnlockConf() { USBKEYPID = ~0; }
    };

    void disable_pll();
    void enable_pll();

    // Allocate a buffer of length N
    uintptr_t bufalloc(int n) {
        const uintptr_t result = _brk;
        _brk += n;
        return result;
    }

    // Return pointer to config for an endpoint.  DIR 0 = OUT
    volatile uint8_t* get_conf(int dir, int n) {
        if (n == 0) {
            return dir ? &USBIEPCNF_0 : &USBOEPCNF_0;
        }
        n--;
        return dir ? (volatile uint8_t*)&USBIEPCNF_1 + n * 8
                : (volatile uint8_t*)&USBOEPCNF_1 + n * 8;
    }

    USB(const USB&);
    const USB& operator=(const USB&);
};

#endif // __MSP430_HAS_USB__
#endif // _USB_DEV_H_
