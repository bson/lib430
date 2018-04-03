// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#include "common.h"
#include "crc16.h"

// Bitwise implementation.  Godawful slow, but it works.
void Crc16::Update(uint8_t d)
{ 
	_crc = _crc ^ (uint16_t(d) << 8);
	
	for (uint i = 0; i < 8; ++i) { 
		if (_crc & 0x8000) 
			_crc = (_crc << 1) ^ 0x1021; 
		else 
			_crc <<= 1; 
	}
} 

void Crc16::Update(const uint8_t* __restrict block, size_t len) __restrict
{
	while (len--)  Update(*block++);
}

uint16_t Crc16::Checksum(const void* block, size_t len, uint16_t initial)
{
	Crc16 crc(initial);
	crc.Update((const uint8_t*)block, len);
	return crc.GetValue();
}
