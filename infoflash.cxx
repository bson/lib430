#include "common.h"
#include "infoflash.h"
#include "task.h"

bool InfoFlash::WriteFlash(uint16_t addr, const void* block, uint16_t len) {
    // Refuse to write outside info blocks
    if (addr < FlashBlock::INFOD || addr >= FlashBlock::INFOB + FlashBlock::INFOB_SIZE)
        return false;

    // If it's the same, don't rewrite it to reduce wear, just succeed
    if (!memcmp((const uint8_t*)addr, block, len))
        return true;

    const uint16_t wdctl = WDTCTL;

    // Stop watchdog timer
    WDTCTL  = WDTPW | WDTHOLD;

    // Make sure it's not busy
    while (FCTL3 & BUSY)
        ;

    FCTL3 = FWPW;         // Unlock
    FCTL1 = FWPW + ERASE; // Enable segment erase
    FCTL4 = FWPW;         // Unlock info flash

    *(uint16_t*)addr = 0; // Dummy write to kick off erase

    // Wait for erase to finish
    while (FCTL3 & BUSY)
        ;

    FCTL3 = FWPW;       // Unlock (XXX redundant?)
    FCTL1 = FWPW + WRT; // Enable segment write
    FCTL4 = FWPW;       // Unlock info flash (XXX redundant?)

    uint16_t* dest = (uint16_t*)addr;
    len /= 2;
    for (const uint16_t* source = (const uint16_t*)block; len > 0; --len) {
        *dest++ = *source++;
    }

    // Lock flash back up
    FCTL1 = FWPW;
    FCTL3 = FWPW+LOCK;
    FCTL4 = FWPW+LOCKINFO;

    // Restore WDT
    WDTCTL = wdctl;

    // Verify result
    return !memcmp((const uint8_t*)addr, block, len);
}
