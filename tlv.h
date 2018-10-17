#ifndef _TLV_H_
#define _TLV_H_

#include <stdint.h>
#include <msp430.h>
#include "cpu/cpu.h"

namespace TLV {

// TLV tags consist of an 8 bit tag code, followed by a length, also
// 8 bits.  Then 'length' number of bytes,

class Tag {
    const uint8_t *addr;  // Tag address (points to length)
public:
    Tag() : addr((const uint8_t*)TLV_BASE) { }
    Tag(const uint8_t* tagaddr) : addr(tagaddr+1) { }
    Tag(const uint16_t* tagaddr) : addr((const uint8_t*)tagaddr + 1) { }

    Tag(const Tag& arg) : addr(arg.addr) { }

    const Tag& operator=(const Tag& rhs) {
        if (&rhs != this)
            addr = rhs.addr;
        return *this;
    }

    ~Tag() { }

    uint8_t code() const { return addr[-1]; }
    uint8_t size() const { return *addr; }
    const uint8_t* data() const { return addr+1; }

    // True if this is the last tag
    bool end() const { return code() == TLV_TAGEND; }

    // Alternate name
    bool not_found() const { return code() == TLV_TAGEND; }

    // Advance to the next tag
    void advance()  { addr += *addr + 2; }
};

// Find the nth of a tag.  Returns end tag if not found.
Tag find(uint8_t tag, int nth = 0);

// Return the memory table base
const uint16_t* mem_base();

// Find a pid
Tag find_pid(uint8_t pid, int nth = 0);

// Return number of known memory segments
int n_mem_segments();

// Return address of memory segment N
uint16_t mem_segment(uint n);

// Peripheral IDs
enum {
    TLV_PID_NO_MODULE = 0x00,
    TLV_PID_PORTMAPPING = 0x10,
    TLV_PID_MSP430CPUXV2 = 0x23,
    TLV_PID_JTAG = 0x09,
    TLV_PID_SBW = 0x0f,
    TLV_PID_EEM_XS = 0x02,
    TLV_PID_EEM_S = 0x03,
    TLV_PID_EEM_M = 0x04,
    TLV_PID_EEM_L = 0x05,
    TLV_PID_PMM = 0x30,
    TLV_PID_PMM_FR = 0x32,
    TLV_PID_FCTL = 0x39,
    TLV_PID_CRC16 = 0x3c,
    TLV_PID_CRC16_RB = 0x3d,
    TLV_PID_WDT_A = 0x40,
    TLV_PID_SFR = 0x41,
    TLV_PID_SYS = 0x42,
    TLV_PID_RAMCTL = 0x44,
    TLV_PID_DMA_1 = 0x46,
    TLV_PID_DMA_3 = 0x47,
    TLV_PID_UCS = 0x48,
    TLV_PID_DMA_6 = 0x4a,
    TLV_PID_DMA_2 = 0x4b,
    TLV_PID_PORT1_2 = 0x51,
    TLV_PID_PORT3_4 = 0x52,
    TLV_PID_PORT5_6 = 0x53,
    TLV_PID_PORT7_8 = 0x54,
    TLV_PID_PORT9_10 = 0x55,
    TLV_PID_PORT11_12 = 0x56,
    TLV_PID_PORTU = 0x5e,
    TLV_PID_PORTJ = 0x5f,
    TLV_PID_TA2 = 0x60,
    TLV_PID_TA3 = 0x61,
    TLV_PID_TA5 = 0x62,
    TLV_PID_TA7 = 0x63,
    TLV_PID_TB3 = 0x65,
    TLV_PID_TB5 = 0x66,
    TLV_PID_TB7 = 0x67,
    TLV_PID_RTC = 0x68,
    TLV_PID_BT_RTC = 0x69,
    TLV_PID_BBS = 0x6a,
    TLV_PID_RTC_B = 0x6b,
    TLV_PID_TD2 = 0x6c,
    TLV_PID_TD3 = 0x6d,
    TLV_PID_TD5 = 0x6e,
    TLV_PID_TD7 = 0x6f,
    TLV_PID_TEC = 0x70,
    TLV_PID_RTC_C = 0x71,
    TLV_PID_AES = 0x80,
    TLV_PID_MPY16 = 0x84,
    TLV_PID_MPY32 = 0x85,
    TLV_PID_MPU = 0x86,
    TLV_PID_USCI_AB = 0x90,
    TLV_PID_USCI_A = 0x91,
    TLV_PID_USCI_B = 0x92,
    TLV_PID_EUSCI_A = 0x94,
    TLV_PID_EUSCI_B = 0x95,
    TLV_PID_REF = 0xa0,
    TLV_PID_COMP_B = 0xa8,
    TLV_PID_COMP_D = 0xa9,
    TLV_PID_USB = 0x98,
    TLV_PID_LCD_B = 0xb1,
    TLV_PID_LCD_C = 0xb2,
    TLV_PID_DAC12_A = 0xc0,
    TLV_PID_SD16_B_1 = 0xc8,
    TLV_PID_SD16_B_2 = 0xc9,
    TLV_PID_SD16_B_3 = 0xca,
    TLV_PID_SD16_B_4 = 0xcb,
    TLV_PID_SD16_B_5 = 0xcc,
    TLV_PID_SD16_B_6 = 0xcd,
    TLV_PID_SD16_B_7 = 0xce,
    TLV_PID_SD16_B_8 = 0xcf,
    TLV_PID_ADC12_A = 0xd1,
    TLV_PID_ADC10_A = 0xd3,
    TLV_PID_ADC10_B = 0xd4,
    TLV_PID_SD16_A = 0xd8,
    TLV_PID_TI_BSL = 0xfc
};

} // ns TLV

#endif // _TLV_H_
