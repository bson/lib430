// Copyright (c) 2020 Jan Brittenson
// See LICENSE for details.

#include <stdint.h>
#include "common.h"
#include "tlv.h"

namespace TLV {

Tag find(uint8_t tag, int nth) {
    Tag t;

    while (!t.end()) {
        if (t.code() == tag) {
            if (!--nth)
                break;
        }
        t.advance();
    }

    return t;
}

const uint16_t* mem_base() {
    static const uint16_t* mbase = NULL;

    if (!mbase)  {
        const Tag mem = find(TLV_PDTAG);
        if (!mem.end())
            mbase = (const uint16_t*)mem.data();
    }
    // This will return NULL if there is no TLV_PDTAG... but I believe all MSP430s
    // have it.  If not, that's why this function returns NULL and the callers go off
    // looking for info in neverland.
    return mbase;
}

int n_mem_segments() {
    static uint8_t nmem = 0;  // # of memory segments

    if (!nmem) {
        const uint8_t* m = (const uint8_t*)mem_base();
        while (*m++) {
            ++m;
            ++nmem;
        }
    }
    return nmem;
}

uint16_t mem_segment(uint n) {
    return mem_base()[n];
}

Tag find_pid(uint8_t pid, int nth) {
    static const uint16_t *pid_base = NULL;

    if (!pid_base)
        pid_base = mem_base() + n_mem_segments() + 1;

    Tag t(pid_base);

    while (!t.end()) {
        if (t.code() == pid) {
            if (!--nth)
                break;
        }
        t.advance();
    }

    return t;
}

} // ns TLV
