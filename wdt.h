#ifndef _WDT_H_
#define _WDT_H_

#include "common.h"
#include "cpu/cpu.h"


namespace WDT {
enum { VECTOR = WDT_VECTOR };

// Initialize WDT at ACLK/8M, but hold and don't start
// E.g. 12M XT2 => ~1.25s
static void init() {
    WDTCTL = WDTPW | WDTHOLD | WDTSSEL__ACLK | WDTIS__8192K | WDTCNTCL;
}

static void stop() {
    WDTCTL  = WDTPW | WDTHOLD;
}

static void start() {
    WDTCTL  = WDTPW | (WDTCTL & ~WDTHOLD) | WDTCNTCL;
}

static void mark() {
    WDTCTL = WDTPW | WDTCTL | WDTCNTCL;
}

// RAII WDT hold (recursively)
class WDTHold {
    const uint16_t save;
public:
    WDTHold() : save(WDTCTL) { }
    ~WDTHold() {
        WDTCTL = WDTPW | save | WDTCNTCL;
    }
};

} // ns WDT

#endif // _WDT_H_
