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
#include "task.h"
#include "util/event.h"

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

    // Events.  Lower values have higher priority.  32 bit mask.
    enum {
        EVENT_NONE     = 0,

        EVENT_RESET    = 1,     // Got reset
        EVENT_INACTIVE = 2,     // Disconnected, waiting for physical connection
        EVENT_ACTIVE   = 8,     // Configured and ready to work
        EVENT_SETUP    = 0x10,  // Setup processed (notification)
        EVENT_SETUPHK  = 0x20,  // Unhandled setup in buffer (HK = hook)
        EVENT_STALL    = 0x40,  // Returned stall (notification)
        EVENT_SETADDR  = 0x80,  // Received set address (notification)
        EVENT_PLL_OOL  = 0x100, // PLL out of lock (notification)
        EVENT_PLL_SOR  = 0x200, // PLL signal or range error (notification)
        EVENT_EP0_OUT  = 0x400, // Received data on EP0 (control data)
        EVENT_READY    = 0x800, // Ready; waiting for SETUP (notification)
        EVENT_SUSPEND  = 0x1000, // Suspended
        EVENT_RESUME   = 0x2000, // Resumed

        // Data receive events
        EVENT_EP1_OUT = 0x10000,
        EVENT_EP2_OUT = 0x20000,
        EVENT_EP3_OUT = 0x40000,
        EVENT_EPx_OUT = 0x80000,  // Any other EP, 4-7 OUT
        EVENT_EP1_IN  = 0x100000,
        EVENT_EP2_IN  = 0x200000,
        EVENT_EP3_IN  = 0x400000,
        EVENT_EPx_IN  = 0x800000,  // Any other EP, 4-7 IN
    };

    // States
    enum State {
        STATE_INACTIVE = 0,
        STATE_ANNOUNCE = 1,
        STATE_READY    = 2,
        STATE_ACTIVE   = 3
    };

    enum {
        DIR_OUT = 0,
        DIR_IN
    };

private:
    static const DeviceDescriptor* _dev_desc;
    static const ConfigDescriptor* _conf_desc;
    static const InterfaceDescriptor* _if_desc;
    static const EndpointDescriptor* _ep_descs;
    static uint8_t _nep_descs;  // # of EP descriptors
    static const char** _strings;
    static uint8_t _nstrings;

    static uint16_t _plldiv;

    static volatile State _state;
    static Event<uint32_t> _events;  // Event object

    static uint16_t _brk;
    static uint8_t _neps;     // Number of endpoint pairs
    static uint8_t _addr;     // Bus address 0-127

public:
    USB() { }

    static void configure(const DeviceDescriptor* dev,
                          const ConfigDescriptor* conf,
                          const InterfaceDescriptor* if_,
                          const EndpointDescriptor* eps,
                          uint8_t neps,
                          const char** strs,
                          uint8_t nstrs,
                          uint16_t plldiv) {
        _dev_desc = dev;
        _conf_desc = conf;
        _if_desc = if_;
        _ep_descs = eps;
        _nep_descs = neps;
        _strings = strs;
        _nstrings = nstrs;
        _plldiv = plldiv;
        _brk = 0;
        _neps = 0;
        _state = STATE_INACTIVE;
    }


    // Get event object
    static Event<uint32_t>& events() { return _events; }

    // Initialize
    static void init() {   }

    static void reset();
    static void start();
    static void suspend();    // Callable by ISR
    static void resume();     // Called by service task
    static void announce();   // Got VBus ON
    static void ready_ack();  // Ready event has been processed by Class and it's ready to setup EPs
    static void enable();     // Class setup and ready

    // Enable an endpoint and allocate buffer for it
    static void add_endpoint(int n, uint16_t rxbuf_size, uint16_t txbuf_size);

    // Begin write to endpoint (device to host transfer)
    static void write_start(int n);

    // Write data to endpoint
    static void write(int n, const void* data, int len);

    // Write to endpoint done, send NULL DATA1 packet
    static void write_done(int n);

    // Write short enough to fit in single 64 byte packet
    static void write_short(int n, const void* data, int len) {
        write_start(n);
        write(n, data, len);
        write_done(n);
    }

    // Read data from endpoint
    static void read(int n, void* data, int& len);

    // Respond with stall on endpoint N
    static void stall(int n);

    // For application to handle SETUP
    static const SetupRequest* get_setup() {
        return (const SetupRequest*)&USBSUBLK;
    }

    // Simply ack endpoint N with a 0 byte data stage
    static void ack(int n) { write(n, NULL, 0); }

    // State inquiries
    static State state() { return _state; }
    static uint8_t addr() { return _addr; }

protected:
    friend void usb_intr();

    static void input_isr(uint16_t endpoint);
    static void output_isr(uint16_t endpoint);
    static void setup_isr();
    static void device_req_isr(const SetupRequest* setup);
    static void interface_req_isr(const SetupRequest* setup);
    static void endpoint_req_isr(const SetupRequest* setup);

private:
    class UnlockConf {
    public:
        UnlockConf() { USBKEYPID = USBKEY; }
        ~UnlockConf() { USBKEYPID = 0x9600; }
    };

    static void disable_pll();
    static void enable_pll();

    // Allocate a buffer of length N
    static uintptr_t bufalloc(int n) {
        const uintptr_t result = _brk;
        _brk += n;
        return result;
    }

    // Return pointer to config for an endpoint.  DIR 0 = OUT
    static volatile uint8_t* get_conf(int dir, int n) {
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
