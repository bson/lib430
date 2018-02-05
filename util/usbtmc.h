#ifndef _USBTMC_H_
#define _USBTMC_H_

#include "config.h"
#include "usb_dev.h"

// Simple USBTMC implementation, sitting on top of the simple USB device.
// Implements the usb488 subclass

// Capabilities response
struct USBTMC_Capabilities {
    uint8_t  status;    // USBTMC status
    uint8_t  res;       // 0
    uint16_t bcdusbtmc; // 0x0100 = USBTMC version 1.0.0
    uint8_t  cap_if;    // Interface capabilities
    uint8_t  cap_dev;   // Device capabilities
    uint8_t  res1[6+12]; // 0
};

template <typename Delegate>
class USBTMC {
    uint8_t _bulk_out_req[65];  // Buffer holding bulk-out commands witb an extra byte
    uint8_t _tag;               // Tag of current request
    int _bulk_out_len;          // Length of bulk-out request
public:
    // Class control requests
    enum {
        INITIATE_ABORT_BULK_OUT = 1,
        CHECK_ABORT_BULK_OUT_STATUS = 2,
        INITIATE_ABORT_BULK_IN = 3,
        CHECK_ABORT_BULK_IN_STATUS = 4,
        INITIATE_CLEAR = 5,
        CHECK_CLEAR_STATUS = 6,
        GET_CAPABILITIES = 7,
        INDICATOR_PULSE = 64,
        READ_STATUS_BYTE = 128, // USB488
        REN_CONTROL = 160,      // USB488
        GO_TO_LOCAL = 161,      // USB488
        LOCAL_LOCKOUT = 162     // USB488
    };

    // USBTMC status
    enum {
        STATUS_SUCCESS = 1,
        STATUS_PENDING = 2,
        STATUS_FAILED = 0x80,
        STATUS_TRANSFER_NOT_IN_PROGRESS = 0x81,
        STATUS_SPLIT_NOT_IN_PROGRESS = 0x82,
        STATUS_SPLIT_IN_PROGRESS = 0x83
    };

    // Base Bulk-OUT and Bulk-IN header (they're the same)
    struct BulkBase {
        uint8_t  msgid;     // Message
        uint8_t  tag;       // Request tag
        uint8_t  taginv;    // Inverse tag
        uint8_t  reserved;  // Reserved
    };

    // Bulk-OUT request and also Bulk-IN response
    struct DevDepBulk {
        BulkBase base;
        uint32_t size;      // Transfer size
        uint8_t  attrs;     // Attributes
        uint8_t  reserved[3];
        uint8_t  data[1];
    } _packed_;

    struct ReqDevDepBulkIn {
        BulkBase base;
        uint32_t size;      // Transfer size
        uint8_t  attrs;     // Attributes
        uint8_t  term;
        uint8_t  reserved[2];
        uint8_t  data[1];
    } _packed_;


    enum {
        ATTR_EOM = 1
    };

    // Messages
    enum {
        // OUT
        DEV_DEP_MSG_OUT = 1,        // Device-dependent OUT with no response
        REQUEST_DEV_DEP_MSG_IN = 2, // Dev dep OUT with DEV_DEP_MSG_IN in return
        VENDOR_SPECIFIC_OUT = 126,  // Vendor specific message OUT with no response
        REQUEST_VENDOR_SPECIFIC_IN = 127, // Vend spec message OUT with VENDOR_SPECIFIC_IN back
        TRIGGER = 128,              // USB488 message (optional, not currently supported)

        // IN
        DEV_DEP_MSG_IN = 2,        // Response to DEV_EP_MSG_OUT
        VENDOR_SPECIFIC_IN = 127   // Response to REQUEST_VENDOR_SPECIFIC_IN
    };

    USBTMC(const char* manuf,
           const char* prod,
           const char* serial,
           uint16_t plldiv);

    void init() { USB::init(); }
    void add_eps();
    void service();
    void control_req(const USB::SetupRequest* setup);
    void bulk_dev_req();

    // Send reply to current Bulk-OUT request
    void reply(const uint8_t* data, int len);

    // Issue IEEE-488 service request
    // XXX implement the full 488.2 SR, ESB, and MAV mechanism
    // See e.g. http://www.ni.com/white-paper/4056/en/
    void srq();

private:
    USBTMC(const USBTMC&);
    USBTMC& operator=(const USBTMC&);
};


#endif // _USBTMC_H_
