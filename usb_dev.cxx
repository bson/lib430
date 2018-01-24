#include "config.h"
#include "cpu/cpu.h"
#include "common.h"
#include "usb_dev.h"
#include "systimer.h"

#include <strings.h>

#ifdef __MSP430_HAS_USB__

void USB::init() {
    UnlockConf u;

    // Enable module and pins
    _sysTimer.wait(TIMER_USEC(10));   // Wait a bit after touching buffer mem before enabling USB
    USBCNF    = USB_EN | PUR_EN;
    USBPHYCTL = PUSEL;  // Use port U for USB

    // 1. Enable VUSB and V18
    USBPWRCTL = SLDOAON | USBDETEN | SLDOEN | VUSBEN;

    // 2. Give power 2ms to start up
    _sysTimer.wait(TIMER_MSEC(2));

    // Enable relevant interrupts
    USBPWRCTL |= VBONIE | VBOFFIE | VUOVLIE;
    USBIE     |= RSTRIE | SUSRIE | RESRIE | SETUPIE | STPOWIE;

    USBMAINT = 0; // Don't use TSGEN or timer

    // Enable EP0 IN
    USBIEPCNF_0 = UBME | USBIIE;
    USBIEPCNT_0 = 0;

    // Enable EP0 OUT
    USBOEPCNF_0 = UBME | USBIIE;
    USBOEPCNT_0 = 0;

    USBIEPIE = 1;
    USBOEPIE = 1;
}

void USB::add_endpoint(int n, uint16_t rxbuf_size, uint16_t txbuf_size) {
    uint8_t *epo_cnf   = (uint8_t*)&USBOEPCNF_1 + (n - 1) * 8;
    uint8_t *epo_xbase = epo_cnf + 1;
    uint8_t *epo_xcnt  = epo_cnf + 2;
    uint8_t *epo_size  = epo_cnf + 7;

    rxbuf_size = (rxbuf_size + 7) & ~7;
    uintptr_t rxbuf = bufalloc(rxbuf_size);
    *epo_cnf   = UBME | USBIIE;
    *epo_xbase = rxbuf >> 3;
    *epo_xcnt  = 0; // Clear any NAK
    *epo_size  = rxbuf_size;

    uint8_t *epi_cnf   = (uint8_t*)&USBIEPCNF_1 + (n - 1) * 8;
    uint8_t *epi_xbase = epi_cnf + 1;
    uint8_t *epi_xcnt  = epi_cnf + 2;
    uint8_t *epi_size  = epi_cnf + 7;

    txbuf_size = (txbuf_size + 7) & ~7;
    uintptr_t txbuf = bufalloc(txbuf_size);
    *epi_cnf   = UBME | USBIIE;
    *epi_xbase = txbuf >> 3;
    *epi_xcnt  = 0;  // Clear any NAK
    *epi_size  = txbuf_size;

    USBIEPIE |= (1 << n);
    USBOEPIE |= (1 << n);

    _neps++;
}

void USB::write(int n, const void* data, int len) {
    if (n == 0) {
        if (len)
            memcpy((void*)&USBIEP0BUF, data, len);
        USBIEPCNT_0 = len;
        return;
    }

    uint8_t *epi_cnf   = (uint8_t*)&USBIEPCNF_1 + (n - 1) * 8;
    void* buf = (void*)(uintptr_t(epi_cnf[1]) << 3);
    uint8_t *epi_xcnt  = epi_cnf + 2;

    if (len)
        memcpy(buf, data, len);
    *epi_xcnt = len; // Also clears NAK for xmit
}

void USB::read(int n, void* data, int& len) {
    if (n == 0) {
        const uint16_t n = USBOEPCNT_0 & 63;
        if (n)
            memcpy(data, (const void*)&USBOEP0BUF, n);
        len = n;
        USBOEPCNT_0 = 0;
        return;
    }

    uint8_t *epo_cnf = (uint8_t*)&USBOEPCNF_1 + (n - 1) * 8;
    const void* buf  = (const void*)(uintptr_t(epo_cnf[1]) << 3);

    uint8_t *epo_xcnt  = epo_cnf + 2;
    const uint16_t nbytes = *epo_xcnt & 63;
    if (nbytes)
        memcpy(data, buf, nbytes);
    len = nbytes;
    *epo_xcnt = 0;   // Clear NAK to facilitate another OUT
}

void USB::stall(int n) {
    if (n == 0) {
        USBIEPCNF_0 |= STALL;
        USBOEPCNF_0 |= STALL;
    } else {
        n--;
        uint8_t *epo_cnf   = (uint8_t*)&USBOEPCNF_1 + n * 8;
        *epo_cnf |= STALL;
        uint8_t *epi_cnf   = (uint8_t*)&USBIEPCNF_1 + n * 8;
        *epi_cnf |= STALL;
    }
    post_event(EVENT_STALL);
}

void USB::input_isr(uint16_t endpoint) {
    // XXX implement events for bulk fill
    // Nothing to do for interrupt endpoints
    ;
}

void USB::output_isr(uint16_t endpoint) {
    static const uint16_t evmap[8] = {
          EVENT_EPx_OUT, EVENT_EP1_OUT, EVENT_EP2_OUT, EVENT_EP3_OUT,
          EVENT_EPx_OUT, EVENT_EPx_OUT, EVENT_EPx_OUT, EVENT_EPx_OUT
    };

    post_event(evmap[endpoint]);
}

void USB::enable_pll() {
    UnlockConf u;

    USBPLLIR = 0; // Clear PLL IRs

    do {
        // 3. Activate the PLL, using the required divider values.
        USBPLLCTL  = UPLLEN;
        USBPLLDIVB = _plldiv;  // Starts PLL?
        _sysTimer.wait(TIMER_MSEC(1));

        USBPLLIR = 0; // Clear PLL IFGs
        _sysTimer.wait(TIMER_MSEC(2));  // Make sure it stays locked

    } while (USBPLLIR & USBOOLIFG);
}

static uint8_t buf[64];

void USB::device_req_isr(const SetupRequest* setup) {

    switch (setup->request) {
    case REQ_GET_STATUS: {
        const uint16_t response = 1; // Self powered, no remote wake
        write(0, &response, 2);
        break;
    }

    case REQ_SET_ADDRESS:
        ack(0);

        // Address is set after ack and status
        _addr = setup->value & 0x7f;
        USBFUNADR = _addr;
        post_event(EVENT_SETADDR);
        break;

    case REQ_GET_DESC: {
        const uint8_t n = setup->index & 0xff;

        switch (setup->value >> 8) {
        case TYPE_DEVICE:
            write(0, _dev_desc, _dev_desc->length);
            break;

        case TYPE_STRING: {
            if (n == 0) {
                static const StringDesc0 s0 _ro_ = {
                    sizeof(StringDesc0), TYPE_STRING, 0x409
                };
                write(0, &s0, sizeof s0);
                break;
            }
            const uint8_t index = n - 1;
            if (index >= _nstrings) {
                stall(0);
                break;
            }

            const char* s = _strings[index];
            const int l = strlen(s);
            buf[0] = 2 + l;
            buf[1] = TYPE_STRING;
            memcpy(buf + 2, s, l);
            write(0, buf, 2 + l);
            break;
        }

        case TYPE_CONFIG: {
            uint8_t* p = buf;
            memcpy(p, _conf_desc, _conf_desc->length);
            p += _conf_desc->length;
            memcpy(p, _if_desc, _if_desc->length);
            p += _if_desc->length;
            const uint8_t n = min<uint8_t>(_nep_descs, 6);
            memcpy(p, _ep_descs, n * sizeof(EndpointDescriptor));
            p += n * sizeof(EndpointDescriptor);
            buf[2] = p - buf;
            write(0, buf, p - buf);
            break;
        }

        case TYPE_INTERFACE:
            write(0, _if_desc, _if_desc->length);
            break;

        case TYPE_ENDPOINT:
            write(0, _ep_descs + n, sizeof(EndpointDescriptor));
            break;

        default:
            stall(0);
            break;
        }
    }
    case REQ_GET_CONF:
        write(0, &_configured, 1);
        break;

    case REQ_SET_CONF:
        if ((setup->value & 0xff) != 1) {
            stall(0);
            break;
        }
        _configured = 1;
        ack(0);
        break;

    case REQ_CLEAR_FEATURE:
    case REQ_SET_FEATURE:
    case REQ_SET_DESC:
        stall(0);
        break;
    default:
        post_event(EVENT_SETUPHK);
        break;
    }

    post_event(EVENT_SETUP);
}

void USB::interface_req_isr(const SetupRequest* setup) {
    switch (setup->request) {
    case REQ_GET_STATUS: {
        const uint16_t response = 0;
        write(0, &response, 2);
        break;
    }

    case REQ_GET_DESC:
    case REQ_GET_CONF:
    case REQ_SET_CONF:
    case REQ_SET_ADDRESS:
    case REQ_CLEAR_FEATURE:
    case REQ_SET_FEATURE:
    case REQ_SET_DESC:
    default:
        stall(0);
        break;
    }

    post_event(EVENT_SETUP);
}

void USB::endpoint_req_isr(const SetupRequest* setup) {
    switch (setup->request) {
    case REQ_GET_STATUS: {
        const uint16_t response = 0;
        write(0, &response, 2);
        break;
    }
    case REQ_GET_DESC:
    case REQ_GET_CONF:
    case REQ_SET_CONF:
    case REQ_SET_ADDRESS:
    case REQ_CLEAR_FEATURE:
    case REQ_SET_FEATURE:
    case REQ_SET_DESC:
    default:
        stall(0);
        break;
    }

    post_event(EVENT_SETUP);
}

void USB::setup_isr() {
    const SetupRequest* setup = (const SetupRequest*)&USBSUBLK;
    const uint8_t recipient = setup->type & 31;

    switch (recipient) {
    case REQ_DEVICE:
        device_req_isr(setup);
        break;
    case REQ_INTERFACE:
        interface_req_isr(setup);
        break;
    case REQ_ENDPOINT:
        endpoint_req_isr(setup);
        break;
    default:
        stall(0);
        break;
    }
}

// Interrupt handler
void _intr_(USB_UBM_VECTOR)  usb_intr() {
    extern USB _usb;

    // Handle this up front so SETUPIFG isn't cleared on reading USBVECINT, since this
    // effectively ends the transaction.  For this reason, the setup command needs to
    // be handled in the ISR.
    if (USBIFG & SETUPIFG) {
        _usb.setup_isr();
        USBIFG &= ~SETUPIFG;
    }

    // Loop through remaining IFGs
    uint16_t cause;
    while ((cause = USBVECINT) != USBVECINT_NONE) {
        switch (cause) {
        case USBVECINT_RSTR:
            _usb.reset_isr();
            break;
        case USBVECINT_SUSR:
            _usb.suspend_isr();
            break;
        case USBVECINT_RESR:
            _usb.resume_isr();
            break;
        case USBVECINT_PWR_DROP:
            // Disconnect?  Ignore.
            break;
        case USBVECINT_PWR_VBUSOn:
            _usb.connect_isr();
            break;
        case USBVECINT_PWR_VBUSOff:
            _usb.disconnect_isr();
            break;
        case USBVECINT_INPUT_ENDPOINT0:
            _usb.input_isr(0);
            break;
        case USBVECINT_OUTPUT_ENDPOINT0:
            _usb.output_isr(0);
            break;
        case USBVECINT_STPOW_PACKET_RECEIVED:
            // Got a setup while processing a setup... just handle it
            _usb.setup_isr();
            break;
        default:
            if (cause >= USBVECINT_INPUT_ENDPOINT1 && cause <= USBVECINT_INPUT_ENDPOINT7) {
                // Enable nested interrupts since we're way down the priority list
                enable_interrupt();

                // Endpoint input
                const uint16_t endpoint = (cause - USBVECINT_INPUT_ENDPOINT1) / 2;
                _usb.input_isr(endpoint);
                break;
            }
            if (cause >= USBVECINT_OUTPUT_ENDPOINT1 && cause <= USBVECINT_OUTPUT_ENDPOINT7) {
                // Enable nexted interrupts since we're way down the priority list
                enable_interrupt();

                // Endpoint output
                const uint16_t endpoint = (cause - USBVECINT_OUTPUT_ENDPOINT1) / 2;
                _usb.output_isr(endpoint);
                break;
            }
            // Ignore everything else
            break;
        }
    }
}



#endif // __MSP430_HAS_USB__
