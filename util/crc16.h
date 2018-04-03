// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef __CRC16_H__
#define __CRC16_H__

#include <stdint.h>
#include <stdlib.h>

// CRC-16 CCITT

class Crc16 {
        uint16_t _crc;
public:
        Crc16(uint16_t initial = 0xffff) : _crc(initial) { }
        void Update(uint8_t byte);
        void Update(const uint8_t* block, size_t len);
        uint16_t GetValue() const { return ~_crc; }

        static uint16_t Checksum(const void* block, size_t len,
        							uint16_t initial = 0xffff);
};

#endif // __CRC16_H__
