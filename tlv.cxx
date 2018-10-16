#include <stdint.h>
#include "common.h"
#include "tlv.h"

namespace TLV {

Tag find(uint8_t tag, int nth) {
    Tag t;
    while (!t.end() && t.code() != tag && nth) {
        t.advance();
        if (t.code() == tag)
            --nth;
    }
    return t;
}

Tag findPid(uint8_t pid, int nth) {
    Tag t = find(TLV_PDTAG);
    if (t.end())
        return t;

    // XXX NYI
    return t;
}

} // ns TLV
