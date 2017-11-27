#include "common.h"
#include "i2c_master/i2c.h"
#include "timer.h"

#pragma CHECK_ULP("none")

template <typename USCI>
void I2CBus<USCI>::init() {
    // Initialize
    USCI::CTL1 |= USCI::SWRST;
    USCI::CTL1 &= ~USCI::SWRST;

    USCI::CTL0 = USCI::MODE_3 | USCI::SYNC | USCI::MST;
    USCI::CTL1 = USCI::SSEL_2;   // SMCLK

    USCI::BR0   = _prescale;
    USCI::BR1   = _prescale >> 8;
}

template <typename USCI>
bool I2CBus<USCI>::start_write(uint8_t addr, uint8_t data) {
    for (int tries = 0; tries < 3; ++tries) {
        init();

        USCI::CTL1 |= USCI::TR;    // transmit mode
        USCI::I2CSA = addr;

        USCI::CTL1 |= USCI::TXSTT;     // send start

        // Output first byte to TXBUF
        USCI::TXBUF = data;

        // Wait for slave ACK (TXSTT clears)
        const uint32_t deadline = _sysTimer.ticks() + TIMER_MSEC(1);
        while ((USCI::CTL1 & USCI::TXSTT) && _sysTimer.ticks() < deadline)
            ;

        if (!(USCI::CTL1 & USCI::TXSTT) && !(USCI::STAT & USCI::NACKIFG)) {
           // Acked: all good
           return true;
        }

        // Still not clear - didn't get ACK - reset bus and retry
        bus_reset();
    }

    // Ran out of retries.  Bus is left reset.
    return false;
}

template <typename USCI>
bool I2CBus<USCI>::write(uint8_t data) {
    if (!wait_tx())
        return false;

    USCI::TXBUF = data;
    return true;
}

template <typename USCI>
void I2CBus<USCI>::write_done() {

    (void)wait_tx();

    // Send stop
    USCI::CTL1 |= USCI::TXSTP;

    // Needed to keep the next start_write from terminating this.  At 100kHz
    // wait 100us.  At 400kHz wait 25us.
    _sysTimer.delay(TIMER_USEC(100*100000/I2C_BUS_SPEED));
}

// * private
template <typename USCI>
bool I2CBus<USCI>::wait_tx() {
    if (USCI::CPU_IFG & USCI::TXIFG) {
        return true;
    }

    const SysTimer::Future f = _sysTimer.future(TIMER_MSEC(3));
    while (!_sysTimer.due(f)) {
        if (USCI::CPU_IFG & USCI::TXIFG) {
            return true;
        }
    }
    bus_reset();
    return false;
}

// * private
template <typename USCI>
void I2CBus<USCI>::bus_reset() {
    ;
}

template <typename USCI>
bool I2CBus<USCI>::start_read(uint8_t slave, uint8_t& data) {
    USCI::CTL1 = USCI::SWRST;
    USCI::CTL0 = USCI::MODE_3 | USCI::SYNC | USCI::MST;
    USCI::CTL1 = USCI::SSEL_2;
    USCI::BR0  = _prescale;
    USCI::BR1  = 0;

    USCI::I2CSA = slave;

    USCI::CTL1 &= ~USCI::TR;   // receive mode

    return true;
}

template <typename USCI>
bool I2CBus<USCI>::read(uint8_t& data) {
    return USCI::RXBUF;
}

template <typename USCI>
void I2CBus<USCI>::read_done() {
    // Send stop
    USCI::CTL1 |= USCI::TXSTP;
}

template <typename USCI>
void I2CDevice<USCI>::write_bytes(const uint8_t *data, size_t len) {
    if (I2CDevice::start_write(data[0])) {
        for (const uint8_t *p = data+1; p < data + len; ) {
            if (!I2CDevice::write(*p++))
                break;
        }
        I2CDevice::write_done();
    }
}

template <typename USCI>
void I2CDevice<USCI>::read_bytes(uint8_t* data, size_t& len) {

}

#pragma RESET_ULP("all")
