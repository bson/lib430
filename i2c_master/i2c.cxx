#ifdef _MAIN_

#include "common.h"
#include "i2c_master/i2c.h"
#include "timer.h"

#pragma CHECK_ULP("none")

template <typename USCI, uint32_t _SPEED>
void I2CBus<USCI,_SPEED>::init() {
    // Initialize
    USCI::CTL1 |= USCI::SWRST;
    USCI::CTL1 &= ~USCI::SWRST;

    USCI::CTL0 = USCI::MODE_3 | USCI::SYNC | USCI::MST;
    USCI::CTL1 = USCI::SSEL_2;   // SMCLK

    USCI::BR0   = PRESCALE;
    USCI::BR1   = PRESCALE >> 8;
}

template <typename USCI, uint32_t _SPEED>
bool I2CBus<USCI,_SPEED>::start_write(uint8_t addr, uint8_t data) {
    for (int tries = 0; tries < 3; ++tries) {
        init();

        USCI::CTL1 |= USCI::TR;    // transmit mode
        USCI::I2CSA = addr;

        USCI::CTL1 |= USCI::TXSTT;     // send start

        // Output first byte to TXBUF
        USCI::TXBUF = data;

        // Wait for slave ACK (TXSTT clears)
        const SysTimer::Future deadline = _sysTimer.future(TIMER_MSEC(1));
        while ((USCI::CTL1 & USCI::TXSTT) && !_sysTimer.due(deadline))
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

template <typename USCI, uint32_t _SPEED>
bool I2CBus<USCI,_SPEED>::write(uint8_t data) {
    if (!wait_tx())
        return false;

    USCI::TXBUF = data;
    return true;
}

template <typename USCI, uint32_t _SPEED>
void I2CBus<USCI,_SPEED>::write_done() {
    (void)wait_tx();

    // Send stop
    USCI::CTL1 |= USCI::TXSTP;

    // XXX Why doesn't this work?  It truncates transfers.
    //wait_done();
	const SysTimer::Future deadline = _sysTimer.future(TIMER_USEC(100*100000/_SPEED));
	while (!_sysTimer.due(deadline))
		;
}

// * private
template <typename USCI, uint32_t _SPEED>
bool I2CBus<USCI,_SPEED>::wait_tx() {
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
template <typename USCI, uint32_t _SPEED>
void I2CBus<USCI,_SPEED>::bus_reset() {
    ;
}

template <typename USCI, uint32_t _SPEED>
bool I2CBus<USCI,_SPEED>::start_read(uint8_t slave, uint8_t* data) {
    for (int tries = 0; tries < 3; ++tries) {
        init();

        USCI::CTL1 &= ~USCI::TR;    // receive mode
        USCI::I2CSA = slave;

        USCI::CTL1 |= USCI::TXSTT;     // send start

        // Wait for slave ACK (TXSTT clears)
        const SysTimer::Future deadline = _sysTimer.future(TIMER_MSEC(1));
        while ((USCI::CTL1 & USCI::TXSTT) && !_sysTimer.due(deadline))
            ;

        if (USCI::STAT & USCI::NACKIFG) {
           // No ACK, try again
           bus_reset();
           continue;
        }

        return read(data);
    }

    // Ran out of retries.  Bus is left reset.
    return false;
}

template <typename USCI, uint32_t _SPEED>
bool I2CBus<USCI,_SPEED>::restart_read(uint8_t slave, uint8_t* data) {
	if (!wait_tx()) {
		return false;
	}

	USCI::CTL1 &= ~USCI::TR;    // receive mode
	USCI::I2CSA = slave;

	USCI::CTL1 |= USCI::TXSTT;     // send start

	// Wait for slave ACK (TXSTT clears)
	const SysTimer::Future deadline = _sysTimer.future(TIMER_MSEC(1));
	while ((USCI::CTL1 & USCI::TXSTT) && !_sysTimer.due(deadline))
		;

	if (USCI::STAT & USCI::NACKIFG) {
		// No ACK, fail
		return false;
	}

	return read(data);
}

template <typename USCI, uint32_t _SPEED>
bool I2CBus<USCI,_SPEED>::wait_rx() {
    const SysTimer::Future deadline = _sysTimer.future(TIMER_MSEC(1));
    while (!(USCI::CPU_IFG & USCI::RXIFG)
    	       && !(USCI::STAT & USCI::NACKIFG)
    		   && !_sysTimer.due(deadline))
    		;

    return USCI::CPU_IFG & USCI::RXIFG;
}

template <typename USCI, uint32_t _SPEED>
bool I2CBus<USCI,_SPEED>::read(uint8_t* data) {
	if (!wait_rx())
		return false;

    *data = USCI::RXBUF;
    return true;
}

template <typename USCI, uint32_t _SPEED>
bool I2CBus<USCI,_SPEED>::read_end(uint8_t* data) {
	// So we NACK-STOP instead of ACK this byte
    USCI::CTL1 |= USCI::TXSTP;

    const bool ok = read(data);
    wait_done();

    return ok;
}

template <typename USCI, uint32_t _SPEED>
void I2CBus<USCI,_SPEED>::read_done() {
    // Send stop
    USCI::CTL1 |= USCI::TXSTP;

    wait_done();
}

template <typename USCI, uint32_t _SPEED>
void I2CBus<USCI,_SPEED>::wait_done() {
	const SysTimer::Future deadline = _sysTimer.future(TIMER_USEC(100*100000/_SPEED));
	while ((USCI::CPU_IFG & USCI::TXSTP) && !_sysTimer.due(deadline))
		;
}

template <typename _Bus, typename USCI>
void I2CDevice<_Bus,USCI>::write_bytes(const uint8_t *data, size_t len) {
    if (start_write(data[0])) {
        for (const uint8_t *p = data+1; p < data + len; ) {
            if (!write(*p++))
                break;
        }
        write_done();
    }
}

template <typename _Bus, typename USCI>
void I2CDevice<_Bus,USCI>::read_bytes(uint8_t* data, size_t& len) {
	size_t n = len;
	if (start_read(data)) {
		while (--n >= 0 && read(data++))
			;
		len -= n;
		read_done();
	}
}

template <typename _Bus, typename USCI>
void I2CDevice<_Bus, USCI>::transmit(uint8_t byte1, uint16_t byte2) {
	if (start_write(byte1)) {
		if (byte2 != 0x100) {
			write(byte2);
		}
		write_done();
	}
}

#pragma RESET_ULP("all")

#endif // _MAIN_
