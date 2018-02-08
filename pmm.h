#ifndef _PMM_H_
#define _PMM_H_

#include "common.h"

namespace PMM {

// Issue reset
static inline void reset() {
    PMMCTL0 |= PMMSWPOR;
}

// This code is adopted from:
// TI slau208o: MSP430 F5xx and 6xx Family Guide, p. 108

// Set Vcore level.  This should only be called once, so make
// it inline other than for debug purposes.
static inline void set_vcore(uint16_t level)
{
    // Open PMM registers for write access
    PMMCTL0_H = 0xa5;

    // Make sure no flags are set for iterative sequences
    while ((PMMIFG & SVSMHDLYIFG) == 0)
        ;

    while ((PMMIFG & SVSMLDLYIFG) == 0)
        ;

    // Set SVS/SVM high side new level
    SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;

    // Set SVM low side to new level
    SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;

    // Wait till SVM is settled
    while ((PMMIFG & SVSMLDLYIFG) == 0)
        ;

    // Clear already set flags
    PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);

    // Set VCore to new level
    PMMCTL0_L = PMMCOREV0 * level;

    // Wait till new level reached
    if ((PMMIFG & SVMLIFG)) {
        while ((PMMIFG & SVMLVLRIFG) == 0)
            ;
    }

    // Set SVS/SVM low side to new level
    SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;

    // Lock PMM registers for write access
    PMMCTL0_H = 0x00;
}

}; // namespace PMM

#endif // _PMM_H_
