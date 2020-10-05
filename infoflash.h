// Copyright (c) 2020 Jan Brittenson
// See LICENSE for details.

#ifndef _INFOFLASH_H_
#define _INFOFLASH_H_

#include "common.h"

// Info flash writer.  No special read is needed, just locate a symbol at
// addr since it's memory mapped.  This can be done for ADDR with:
//
//   Data _data __attribute__((location(ADDR), noinit));
//
// Where Data is a struct or other suitable POD type.

class InfoFlash {
public:
    // Write a block of flash.  Addr and block should be word aligned and len
    // a multiple of 2.  Flash info segment can be B and up; segment A can't be
    // written by this (it will reject the address and in either case won't clear
    // the LOCKA bit to unlock it).
    static bool WriteFlash(uint16_t addr, const void* block, uint16_t len);
};
#endif // _INFOFLASH_H_
