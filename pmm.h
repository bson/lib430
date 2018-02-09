#ifndef _PMM_H_
#define _PMM_H_

#include "common.h"

namespace PMM {

// Issue reset.  Mainly for use by panic().
static inline void reset() {
    PMMCTL0 |= PMMSWPOR;
}


// Set Vcore level.  Shamelessly borrowed from the TI USB demo.
static inline bool vcore_up(uint8_t level) {
    PMMCTL0_H = 0xa5;

    //The code flow for increasing the Vcore has been altered to work around
    //the erratum FLASH37.
    //Please refer to the Errata sheet to know if a specific device is affected

    // Initialize and save some previous values
    uint16_t pmmrie_save = PMMRIE;
    PMMRIE &= ~(SVMHVLRPE | SVSHPE | SVMLVLRPE | SVSLPE | SVMHVLRIE | SVMHIE |
            SVSMHDLYIE | SVMLVLRIE | SVMLIE | SVSMLDLYIE);

    uint16_t svsmhctl_save = SVSMHCTL;
    uint16_t svsmlctl_save = SVSMLCTL;
    PMMIFG = 0;

    //Set SVM highside to new level and check if a VCore increase is possible
    SVSMHCTL = SVMHE | SVSHE | (SVSMHRRL0 * level);
    while (!(PMMIFG & SVSMHDLYIFG))
        ;

    PMMIFG &= ~SVSMHDLYIFG;

    //Check if a VCore increase is possible
     if ((PMMIFG & SVMHIFG) == SVMHIFG) {
         //-> Vcc is too low for a Vcore increase
         //recover the previous settings
         PMMIFG &= ~SVSMHDLYIFG;
         SVSMHCTL = svsmhctl_save;

         //Wait until SVM highside is settled
         while (!(PMMIFG & SVSMHDLYIFG))
             ;

         //Clear all Flags
         PMMIFG &= ~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
                 SVMLVLRIFG | SVMLIFG | SVSMLDLYIFG);

         //Restore PMM interrupt enable register
         PMMRIE = pmmrie_save;

         //Lock PMM registers for write access
         PMMCTL0_H = 0x00;

         //return: voltage not set
         return false;
     }

     //Set also SVS highside to new level
     //Vcc is high enough for a Vcore increase
     SVSMHCTL |= SVSHRVL0 * level;

     //Wait until SVM highside is settled
     while (!(PMMIFG & SVSMHDLYIFG))
         ;

     //Clear flag
     PMMIFG &= ~SVSMHDLYIFG;

     //Set VCore to new level
     PMMCTL0_L = PMMCOREV0 * level;

     //Set SVM, SVS low side to new level
     SVSMLCTL = SVMLE | (SVSMLRRL0 * level) | SVSLE | (SVSLRVL0 * level);

     //Wait until SVM, SVS low side is settled
     while (!(PMMIFG & SVSMLDLYIFG))
         ;

     //Clear flag
     PMMIFG &= ~SVSMLDLYIFG;

     //SVS, SVM core and high side are now set to protect for the new core level

     //Restore Low side settings
     //Clear all other bits _except_ level settings
     SVSMLCTL &= SVSLRVL0 | SVSLRVL1 | SVSMLRRL0 | SVSMLRRL1 | SVSMLRRL2;

     //Clear level settings in the backup register,keep all other bits
     svsmlctl_save &= ~(SVSLRVL0 | SVSLRVL1 | SVSMLRRL0 | SVSMLRRL1 | SVSMLRRL2);

     //Restore low-side SVS monitor settings
     SVSMLCTL |= svsmlctl_save;

     //Restore High side settings
     //Clear all other bits except level settings
     SVSMHCTL &= SVSHRVL0 | SVSHRVL1 | SVSMHRRL0 | SVSMHRRL1 | SVSMHRRL2;

     //Clear level settings in the backup register,keep all other bits
     svsmhctl_save &= ~(SVSHRVL0 | SVSHRVL1 | SVSMHRRL0 | SVSMHRRL1 | SVSMHRRL2);

     //Restore backup
     SVSMHCTL |= svsmhctl_save;

     //Wait until high side, low side settled
     while (!(PMMIFG & SVSMLDLYIFG) || !(PMMIFG & SVSMHDLYIFG))
         ;

     //Clear all Flags
     PMMIFG &= ~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
             SVMLVLRIFG | SVMLIFG | SVSMLDLYIFG);

     //Restore PMM interrupt enable register
     PMMRIE = pmmrie_save;

     //Lock PMM registers for write access
     PMMCTL0_H = 0x00;

     return true;
}

static bool vcore_down(uint8_t level) {
    uint32_t PMMRIE_backup, SVSMHCTL_backup, SVSMLCTL_backup;

    //The code flow for decreasing the Vcore has been altered to work around
    //the erratum FLASH37.
    //Please refer to the Errata sheet to know if a specific device is affected
    //DO NOT ALTER THIS FUNCTION

    //Open PMM registers for write access
    PMMCTL0_H = 0xA5;

    //Disable dedicated Interrupts
    //Backup all registers
    PMMRIE_backup = PMMRIE;
    PMMRIE &= ~(SVMHVLRPE | SVSHPE | SVMLVLRPE | SVSLPE | SVMHVLRIE | SVMHIE |
            SVSMHDLYIE | SVMLVLRIE | SVMLIE | SVSMLDLYIE);

    SVSMHCTL_backup = SVSMHCTL;
    SVSMLCTL_backup = SVSMLCTL;

    //Clear flags
    PMMIFG &= ~(SVMHIFG | SVSMHDLYIFG | SVMLIFG | SVSMLDLYIFG);

    //Set SVM, SVS high & low side to new settings in normal mode
    SVSMHCTL = SVMHE | (SVSMHRRL0 * level) | SVSHE | (SVSHRVL0 * level);
    SVSMLCTL = SVMLE | (SVSMLRRL0 * level) | SVSLE | (SVSLRVL0 * level);

    //Wait until SVM high side and SVM low side is settled
    while (!(PMMIFG & SVSMHDLYIFG) || !(PMMIFG & SVSMLDLYIFG))
        ;

    //Clear flags
    PMMIFG &= ~(SVSMHDLYIFG + SVSMLDLYIFG);
    //SVS, SVM core and high side are now set to protect for the new core level

    //Set VCore to new level
    PMMCTL0_L = PMMCOREV0 * level;

    //Restore Low side settings
    //Clear all other bits _except_ level settings
    SVSMLCTL &= SVSLRVL0 | SVSLRVL1 | SVSMLRRL0 | SVSMLRRL1 | SVSMLRRL2;

    //Clear level settings in the backup register,keep all other bits
    SVSMLCTL_backup &= ~(SVSLRVL0 | SVSLRVL1 | SVSMLRRL0 | SVSMLRRL1 | SVSMLRRL2);

    //Restore low-side SVS monitor settings
    SVSMLCTL |= SVSMLCTL_backup;

    //Restore High side settings
    //Clear all other bits except level settings
    SVSMHCTL &= SVSHRVL0 | SVSHRVL1 | SVSMHRRL0 | SVSMHRRL1 | SVSMHRRL2;

    //Clear level settings in the backup register, keep all other bits
    SVSMHCTL_backup &= ~(SVSHRVL0 | SVSHRVL1 | SVSMHRRL0 | SVSMHRRL1 | SVSMHRRL2);

    //Restore backup
    SVSMHCTL |= SVSMHCTL_backup;

    //Wait until high side, low side settled
    while(!(PMMIFG & SVSMLDLYIFG) || !(PMMIFG & SVSMHDLYIFG))
        ;

    //Clear all Flags
    PMMIFG &= ~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
            SVMLVLRIFG | SVMLIFG | SVSMLDLYIFG);

    //Restore PMM interrupt enable register
    PMMRIE = PMMRIE_backup;
    //Lock PMM registers for write access
    PMMCTL0_H = 0x00;
    //Return: OK
    return true;
}

static bool set_vcore(uint8_t level) {
    uint8_t actlevel;
    bool status = true;

    //Set Mask for Max. level
    level &= PMMCOREV_3;

    //Get actual VCore
    actlevel = PMMCTL0 & PMMCOREV_3;

    //step by step increase or decrease
    while ((level != actlevel) && status) {
        if (level > actlevel) {
            status = vcore_up(++actlevel);
        } else {
            status = vcore_down(--actlevel);
        }
    }

    return status;
}

}; // namespace PMM

#endif // _PMM_H_
