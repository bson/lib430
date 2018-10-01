#include "common.h"

class InfoFlash {
public:
    // Write a block of flash.  Addr and block should be word aligned and len
    // a multiple of 2.  Flash info segment can be B and up; segment A can't be
    // written by this (it will reject the address and in either case won't clear
    // the LOCKA bit to unlock it).
    static bool WriteFlash(uint16_t addr, const void* block, uint16_t len);

    // Read as block of flash
    static bool ReadFlash(uint16_t addr, void* block, uint16_t len) {
        memcpy(block, (const uint8_t*)addr, len);
        return true;
    }
};
