#include <stdint.h>
#include <lib430/common.h>
#include "display.h"

void Display::init() {
	TextPanel::init(true);

	TextPanel::setpos(1, 10); TextPanel::puts("PSU Board Rev 1");
	TextPanel::setpos(2, 10); TextPanel::puts("Initializing...");
}

void Display::refresh() {

}

void Display::update_set_voltage(uint32_t v) {

}

void Display::update_set_current(uint32_t i) {

}

void Display::update_read_voltage(uint32_t v) {

}

void Display::update_read_current(uint32_t i) {

}
