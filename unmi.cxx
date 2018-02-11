#include "common.h"
#include "task.h"


void _intr_(UNMI_VECTOR) unmi_isr() {
    switch(__even_in_range(SYSUNIV, 0x08)) {
    case 0x00: break;
    case 0x02: break;
    case 0x04: break;
    case 0x06: break;
    case 0x08: {
        // Bus error
        Task::Exception::post(Task::Exception::CODE_BERR);

        // NMIIFG
        // OFIFG
        // ACCVIFG
        // BUSIFG
        // If needed, obtain the flash error location here.
//        unsigned short* ErrorLocation = MidGetErrAdr();
        switch(__even_in_range(SYSBERRIV, 0x08)) {
        case 0x00: break;
        case 0x02: break;
        case 0x04: break;
        case 0x06:
            // mid error
            break;
        case 0x08: break;
        default:   break;
        }
        break;
    }
    default:   break;
    }
}
