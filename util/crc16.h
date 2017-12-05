#ifndef __CRC16_H__
#define __CRC16_H__

#include <stdint.h>
#include <stdlib.h>

// CRC-16 CCITT

class Crc16 {
        uint16_t _crc;

        const static uint16_t _crc_table[256];
public:
        // SD cards appear to use an initial value of 0 and use the sum, whereas
        // a proper CCITT CRC-16 use a value of 0xffff and use the 1's complement.
        Crc16(uint16_t initial = 0) : _crc(initial) { }
        void Update(uint8_t byte);
        void Update(const uint8_t* block, size_t len);
        uint16_t GetValue() const { return _crc; }

        static uint16_t Checksum(const void* block, size_t len, uint16_t initial = 0);
};

#endif // __CRC16_H__
