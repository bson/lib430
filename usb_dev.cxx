#include "config.h"
#include "cpu/cpu.h"
#include "common.h"
#include "usb_dev.h"
#include "systimer.h"

#include <strings.h>

#ifdef __MSP430_HAS_USB__

const USB::DeviceDescriptor* USB::_dev_desc;
const USB::ConfigDescriptor* USB::_conf_desc;
const USB::InterfaceDescriptor* USB::_if_desc;
const USB::EndpointDescriptor* USB::_ep_descs;
uint8_t USB::_nep_descs;  // # of EP descriptors
const char** USB::_strings;
uint8_t USB::_nstrings;

uint16_t USB::_plldiv;

volatile uint32_t USB::_events;  // Event mask

volatile USB::State USB::_state;

uint16_t USB::_brk;
uint8_t USB::_neps;     // Number of endpoint pairs
uint8_t USB::_addr;     // Bus address 0-127

void USB::reset() {
    NoInterruptReent g;

    _state = STATE_INACTIVE;
    _events &= EVENT_RESET; // Drop all pending events except EVENT_RESET

    UnlockConf u;

    // Errata: USB9
    USBPWRCTL  = 0;

    USBPLLIR  &= ~(USBOOLIE | USBLOSIE | USBOORIE);
    USBPLLCTL &= ~UPLLEN; // Turn off PLL
    USBPWRCTL  = VUSBEN | SLDOAON;

    // 1ms delay
    __delay_cycles(MCLK / 1000 * 1);

    USBFUNADR  = 0;
    USBCNF     = 0;
    USBPHYCTL  = PUSEL;
    USBPLLIR   = 0;      // Disable IE, clear IFG
    USBIE      = 0;      // No other interrupts
    USBIFG     = 0;      // Drop all pending interrupts
    USBIEPIE   = 0;      // Disable all EPx IN interrupts
    USBOEPIE   = 0;      // Disable all EPx OUT interrupts
    USBCTL     = FEN;    // Enable USB function
    USBPWRCTL |= VBONIE; // Enable VBusOn interrupt
    USBIEPIE   = 0;      // Disable EP intrs
    USBOEPIE   = 0;      // Disable EP intrs

    post_event(EVENT_INACTIVE);
}

void USB::announce() {
    NoInterruptReent g;
    UnlockConf u;

    USBPWRCTL &= ~VBONIE;
    USBPWRCTL |= VBOFFIE;
    USBIE      = RSTRIE; // Interrupts: reset

    _state = STATE_ANNOUNCE;

    post_event(EVENT_READY);
}

void USB::suspend() {
    NoInterruptReent g;
    UnlockConf u;

    USBPLLIR   = 0;       // Disable PLL IE, clear IFG
    USBPLLCTL &= ~UPLLEN; // Turn off PLL
    USBIE      = RSTRIE | RESRIE;
}

void USB::resume() {
    NoInterruptReent g;

    enable_pll();

    // Interrupts: VBusOff, Reset, Suspend, Setup, Setup overwrite, PLL
    USBPWRCTL &= ~VBONIE;
    USBPWRCTL |= VBOFFIE;
    USBIE      = RSTRIE | SUSRIE | SETUPIE /*| STPOWIE*/;
    USBPLLIR  |= USBOOLIE | USBLOSIE | USBOORIE;

    USBCTL |= FEN;

    USBCNF |= PUR_EN;   // Pull-up on DP to let host know we're here
}

void USB::ready_ack() {
    enable_pll();

    _sysTimer.wait(TIMER_USEC(20));

    NoInterrupt g;

    // Clear USB buffer memory
    memset((void*)&USBSTABUFF, 0, &USBTOPBUFF-&USBSTABUFF+24);  // Clear all USB buffer mem

    UnlockConf u;

    // Enable EP0 IN, DATA0
    USBIEPCNF_0 |= UBME | USBIIE | STALL;
    USBIEPCNT_0  = NAK;

    // Enable EP0 OUT, DATA1
    USBOEPCNF_0 |= UBME | USBIIE | STALL | TOGGLE;
    USBOEPCNT_0  = NAK;

    USBIEPIE     = BIT0;   // EP0 input transaction interrupts
    USBOEPIE     = BIT0;   // EP0 output transaction interrupts

    // Interrupts: VBusOff, Reset, Suspend, Setup, Setup overwrite, PLL
    USBPWRCTL &= ~VBONIE;
    USBPWRCTL |= VBOFFIE;
    USBIFG     = 0;       // Clear any pending interrupts
    USBIE      = /*RSTRIE | *//* SUSRIE |*/ SETUPIE /*| STPOWIE*/;
    USBPLLIR  |= USBOOLIE | USBLOSIE | USBOORIE;

    USBCTL  = FEN;

    USBCNF |= PUR_EN;   // Pull-up on DP to let host know we're here

    _brk   = 0;
    _neps  = 0;

    _state = STATE_READY;
}

void USB::enable_pll() {
    // Ignore if PLL is already running
    if ((USBCNF & USB_EN) && (USBPLLCTL & UPLLEN)) {
        return;
    }

    UnlockConf u;

    USBPLLIR = 0; // Disable PLL IE and clear IFGs

    // 3. Activate the PLL, using the required divider values.
    USBPLLCTL  = UPLLEN | UPFDEN;
    USBPLLDIVB = _plldiv;

    // Workaorund for USB8 errata - briefly enable DCO or USB PLL may not start
    const uint16_t ucs4 = UCSCTL4;
    UCSCTL4 = SELA__XT2CLK + SELS__XT2CLK + SELM__DCOCLK; // Enable the DCO
    _sysTimer.wait(TIMER_MSEC(1));
    UCSCTL4 = ucs4;

    do {
        USBPLLIR = 0; // Clear PLL IFGs
        _sysTimer.wait(TIMER_MSEC(1));
    } while (USBPLLIR);

    USBCNF |= USB_EN;   // USB module memory access enable
}

void USB::add_endpoint(int n, uint16_t rxbuf_size, uint16_t txbuf_size) {
    volatile uint8_t *epo_cnf   = get_conf(n, DIR_OUT);
    volatile uint8_t *epo_xbase = epo_cnf + 1;
    volatile uint8_t *epo_xcnt  = epo_cnf + 2;
    volatile uint8_t *epo_size  = epo_cnf + 7;

    rxbuf_size = (rxbuf_size + 7) & ~7;
    uintptr_t rxbuf = bufalloc(rxbuf_size);
    *epo_cnf   = UBME | USBIIE;
    *epo_xbase = rxbuf >> 3;
    *epo_xcnt  = 0; // Clear any NAK
    *epo_size  = rxbuf_size;

    volatile uint8_t *epi_cnf   = get_conf(n, DIR_IN);
    volatile uint8_t *epi_xbase = epi_cnf + 1;
    volatile uint8_t *epi_xcnt  = epi_cnf + 2;
    volatile uint8_t *epi_size  = epi_cnf + 7;

    txbuf_size = (txbuf_size + 7) & ~7;
    uintptr_t txbuf = bufalloc(txbuf_size);
    *epi_cnf   = UBME | USBIIE;
    *epi_xbase = txbuf >> 3;
    *epi_xcnt  = NAK;
    *epi_size  = txbuf_size;

    USBIEPIE |= (1 << n);
    USBOEPIE |= (1 << n);

    _neps++;
}

void USB::write_start(int n) {
    volatile uint8_t *conf = get_conf(n, DIR_IN);
    *conf |= TOGGLE;
}

void USB::write(int n, const void* data, int len) {
    if (n == 0) {
        if (len)
            memcpy((void*)&USBIEP0BUF, data, len);

        USBIEPCNT_0 = len;
        return;
    }

    volatile uint8_t *conf = get_conf(n, DIR_IN);
    void* buf = (void*)(uintptr_t(conf[1]) << 3);
    volatile uint8_t *count  = conf + 2;

    if (len)
        memcpy(buf, data, len);

    *count = len; // Also clears NAK for xmit
}

void USB::write_done(int n) {
    volatile uint8_t *conf = get_conf(n, DIR_OUT);
    volatile uint8_t *count  = conf + 2;
    *conf |= TOGGLE;
    *count = 0;
}

void USB::read(int n, void* data, int& len) {
    if (n == 0) {
        const uint16_t nbytes = USBOEPCNT_0 & 63;
        if (nbytes)
            memcpy(data, (const void*)&USBOEP0BUF, nbytes);
        len = nbytes;
        USBOEPCNT_0 = 0;
        return;
    }

    volatile uint8_t *conf = get_conf(n, DIR_OUT);
    const void* buf  = (const void*)(uintptr_t(conf[1]) << 3);
    volatile uint8_t *count  = conf + 2;
    const uint16_t nbytes = *count & 63;
    if (nbytes)
        memcpy(data, buf, nbytes);
    len = nbytes;
    *count = 0;   // Clear NAK to facilitate another OUT
}

void USB::stall(int n) {
    *get_conf(n, DIR_OUT) |= STALL;
    *get_conf(n, DIR_IN)  |= STALL;

    post_event(EVENT_STALL);
}

void USB::input_isr(uint16_t endpoint) {
    static const uint32_t evmap[8] = {
          EVENT_EPx_IN, EVENT_EP1_IN, EVENT_EP2_IN, EVENT_EP3_IN,
          EVENT_EPx_IN, EVENT_EPx_IN, EVENT_EPx_IN, EVENT_EPx_IN
    };

    post_event(evmap[endpoint]);
}

void USB::output_isr(uint16_t endpoint) {
    static const uint32_t evmap[8] = {
          EVENT_EP0_OUT, EVENT_EP1_OUT, EVENT_EP2_OUT, EVENT_EP3_OUT,
          EVENT_EPx_OUT, EVENT_EPx_OUT, EVENT_EPx_OUT, EVENT_EPx_OUT
    };

    post_event(evmap[endpoint]);
}

static uint8_t buf[64];

void USB::device_req_isr(const SetupRequest* setup) {

    switch (setup->request) {
    case REQ_GET_STATUS: {
        const uint16_t response = 1; // Self powered, no remote wake
        write_short(0, &response, 2);
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
            write_short(0, _dev_desc, _dev_desc->length);
            break;

        case TYPE_STRING: {
            if (n == 0) {
                static const StringDesc0 s0 = {
                    sizeof(StringDesc0), TYPE_STRING, 0x409
                };
                write_short(0, &s0, sizeof s0);
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
            write_short(0, buf, 2 + l);
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
            write_short(0, buf, p - buf);
            break;
        }

        case TYPE_INTERFACE:
            write_short(0, _if_desc, _if_desc->length);
            break;

        case TYPE_ENDPOINT:
            write_short(0, _ep_descs + (n & 0xf) - 1, sizeof(EndpointDescriptor));
            break;

        default:
            stall(0);
            break;
        }
    }
    case REQ_GET_CONF: {
        const uint8_t configured = (_state == STATE_ACTIVE);
        write_short(0, &configured, 1);
        break;
    }
    case REQ_SET_CONF:
        if ((setup->value & 0xff) != 1) {
            stall(0);
            break;
        }
        _state = STATE_ACTIVE;
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
}

void USB::interface_req_isr(const SetupRequest* setup) {
    switch (setup->request) {
    case REQ_GET_STATUS: {
        const uint16_t response = 0;
        write_short(0, &response, 2);
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
}

void USB::endpoint_req_isr(const SetupRequest* setup) {
    switch (setup->request) {
    case REQ_GET_STATUS: {
        const uint16_t response = 0;
        write_short(0, &response, 2);
        break;
    }

    case REQ_CLEAR_FEATURE:
    case REQ_SET_FEATURE: {
        if (setup->value == 0x00) {
            const int ep = setup->index & 0x0f;
            if (ep != 0) {
                volatile uint8_t* ep_cnf = get_conf(setup->index & 0x80, ep);
                if (setup->request == REQ_CLEAR_FEATURE) {
                    *ep_cnf &= ~STALL;
                } else {
                    *ep_cnf |= STALL;
                }
            }
        }
    }

    case REQ_GET_DESC:
    case REQ_GET_CONF:
    case REQ_SET_CONF:
    case REQ_SET_ADDRESS:
    case REQ_SET_DESC:
    default:
        stall(0);
        break;
    }
}

void USB::setup_isr() {
    const SetupRequest* setup = (const SetupRequest*)&USBSUBLK;
    const uint8_t recipient = setup->type & 31;
    const uint8_t type = (setup->type >> 5) & 3;

    if (type & 0x80) {
        USBCTL |= DIR;  // IN
    } else {
        USBCTL &= ~DIR; // OUT
    }

    if (type == 1 || type == 2) {
        // Class/vendor based request... pass on
        post_event(EVENT_SETUPHK);
        return;
    }
    if (type != 0) {
        stall(0);
        return;
    }
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
void _intr_(USB_UBM_VECTOR) usb_intr() {
    // Handle this up front so SETUPIFG isn't cleared on reading USBVECINT, since this
    // effectively ends the transaction.  For this reason, the setup command needs to
    // be handled in the ISR.
    if (USBIFG & SETUPIFG) {
        USB::post_event(USB::EVENT_SETUP);

        USB::setup_isr();

        // Clear SETUPIFG (errata USB10 workaround)
        USBIEPCNF_0 &= ~UBME; // Clear ME to gate off SETUPIFG clear event
        USBOEPCNF_0 &= ~UBME; // Clear ME to gate off SETUPIFG clear event
        USBIFG &= ~SETUPIFG; // clear the interrupt bit
        USBIEPCNF_0 |= UBME; // Set ME to continue with normal operation
        USBOEPCNF_0 |= UBME; // Set ME to continue with normal operation
    }

    // Loop through remaining IFGs
    uint16_t cause;
    while ((cause = USBVECINT) != USBVECINT_NONE) {
        switch (cause) {
        case USBVECINT_SETUP_PACKET_RECEIVED:
            // XXX for debugging, shouldn't get here
            USB::setup_isr();
            USB::post_event(USB::EVENT_SETUP);
            break;
        case USBVECINT_RSTR:
            USB::post_event(USB::EVENT_RESET);
            USB::reset();
            break;
        case USBVECINT_SUSR:
            USB::suspend();
            break;
        case USBVECINT_RESR:
            USB::resume();
            break;
        case USBVECINT_PWR_DROP:
            USB::reset();
            break;
        case USBVECINT_PWR_VBUSOn:
            USB::announce();
            break;
        case USBVECINT_PWR_VBUSOff:
            USB::reset();
            break;
        case USBVECINT_INPUT_ENDPOINT0:
            USB::input_isr(0);
            break;
        case USBVECINT_OUTPUT_ENDPOINT0:
            USB::output_isr(0);
            break;
        case USBVECINT_STPOW_PACKET_RECEIVED:
            USB::stall(0);
            break;
        case USBVECINT_PLL_LOCK:
            USB::post_event(USB::EVENT_PLL_OOL);
            break;
        case USBVECINT_PLL_SIGNAL:
        case USBVECINT_PLL_RANGE:
            USB::post_event(USB::EVENT_PLL_SOR);
            break;
        default:
            if (cause >= USBVECINT_INPUT_ENDPOINT1 && cause <= USBVECINT_INPUT_ENDPOINT7) {
                // Enable nested interrupts since we're way down the priority list
                //enable_interrupt();

                // Endpoint input
                const uint16_t endpoint = (cause - USBVECINT_INPUT_ENDPOINT1) / 2;
                USB::input_isr(endpoint);
                break;
            }
            if (cause >= USBVECINT_OUTPUT_ENDPOINT1 && cause <= USBVECINT_OUTPUT_ENDPOINT7) {
                // Enable nexted interrupts since we're way down the priority list
                //enable_interrupt();

                // Endpoint output
                const uint16_t endpoint = (cause - USBVECINT_OUTPUT_ENDPOINT1) / 2;
                USB::output_isr(endpoint);
                break;
            }
            // Ignore everything else
            break;
        }
    }
}

#endif // __MSP430_HAS_USB__
