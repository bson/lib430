#include "wdt.h"

void _intr_(WDT::VECTOR) wdt_isr() {
    WDT::mark();
}
