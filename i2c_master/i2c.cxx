#include <msp430.h>
#include <stdint.h>
#include <stddef.h>

#include "i2c_master/i2c.h"
#include "timer.h"

#pragma CHECK_ULP("none")

void I2CBus::init() {
    // Initialize
    UCB0CTL1 |= UCSWRST;
    UCB0CTL1 &= ~UCSWRST;

    UCB0CTL0 = UCMODE_3 | UCSYNC | UCMST;
    UCB0CTL1 = UCSSEL_2;   // SMCLK

    UCB0BR0   = _prescale;
    UCB0BR1   = _prescale >> 8;
}

bool I2CBus::start_write(uint8_t addr, uint8_t data) {
    for (int tries = 0; tries < 3; ++tries) {
        init();

        UCB0CTL1 |= UCTR;    // transmit mode
        UCB0I2CSA = addr;

        UCB0CTL1 |= UCTXSTT;     // send start

        // Output first byte to TXBUF
        UCB0TXBUF = data;

        // Wait for slave ACK (TXSTT clears)
        const uint32_t deadline = _timer.ticks() + TIMER_MSEC(1);
        while ((UCB0CTL1 & UCTXSTT) && _timer.ticks() < deadline)
            ;

        if (!(UCB0CTL1 & UCTXSTT) && !(UCB0STAT & UCNACKIFG)) {
           // Acked: all good
           return true;
        }

        // Still not clear - didn't get ACK - reset bus and retry
        bus_reset();
    }

    // Ran out of retries.  Bus is left reset.
    return false;
}

bool I2CBus::write(uint8_t data) {
    if (!wait_tx())
        return false;

    UCB0TXBUF = data;
    return true;
}

void I2CBus::write_done() {

    (void)wait_tx();

    // Send stop
    UCB0CTL1 |= UCTXSTP;

    // Needed to keep the next start_write from terminating this.  At 100kHz
    // wait 100us.  At 400kHz wait 25us.
    _timer.delay(TIMER_USEC(100*100000/I2C_BUS_SPEED));
}

// * private
bool I2CBus::wait_tx() {
    if (UC0IFG & UCB0TXIFG) {
        return true;
    }

    const uint32_t deadline = _timer.ticks() + TIMER_MSEC(3);
    while (_timer.ticks() < deadline) {
        if (UC0IFG & UCB0TXIFG) {
            return true;
        }
    }
    bus_reset();
    return false;
}

// * private
void I2CBus::bus_reset() {
    ;
}

void I2CBus::start(uint8_t slave, bool read) {
    UCB0CTL1 = UCSWRST;
    UCB0CTL0 = UCMODE_3 | UCSYNC | UCMST;
    UCB0CTL1 = UCSSEL_2;
    UCB0BR0  = _prescale;
    UCB0BR1  = 0;

    UCB0I2CSA = slave;

    if (read) {
        UCB0CTL1 &= ~UCTR;   // receive mode
    } else {
        UCB0CTL1 |= UCTR;    // transmit mode
    }
    UCB0CTL1 |= UCTXSTT;     // send start
}

void I2CBus::send(uint8_t v) {
    UCB0TXBUF = v;
}    

uint8_t I2CBus::recv() {
    wait(SIG_RX);
    return UCB0RXBUF;
}

void I2CBus::stop() {
    // Wait for UCTXSTT to clear; this is only relevant if we sent a single
    // byte since it clears when the first byte is about to go out on the wire.
    // If we fire off a stop too soon we're effectively aborting the transmit and
    // only the slave address is sent.
    while (UCB0CTL1 & UCTXSTT)
        ;
    UCB0CTL1 |= UCTXSTP;         // stop
}

void I2CBus::wait(uint8_t mask) {
    uint8_t ifg = 0;
    if (mask & SIG_TX) {
        ifg |= UCB0TXIFG;
    }
    if (mask & SIG_RX) {
        ifg |= UCB0RXIFG;
    }
    while (!(UC0IFG & ifg))
        ;
    UC0IFG &= ~ifg;
}

void I2CDevice::transmit(uint8_t byte1, uint16_t byte2) {
    if (start_write(byte1)) {
        if (byte2 < 0x100) {
            _i2c_bus_master.write((uint8_t)byte2);
        }
        _i2c_bus_master.write_done();
    }
}
void I2CDevice::write_bytes(const uint8_t *data, size_t len) {
    if (I2CDevice::start_write(data[0])) {
        for (const uint8_t *p = data+1; p < data + len; ) {
            if (!I2CDevice::write(*p++))
                break;
        }
        I2CDevice::write_done();
    }
}

void I2CDevice::wait_for_idle() {
	while (busy())
		;
}

#pragma RESET_ULP("all")
