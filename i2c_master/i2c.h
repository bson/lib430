#ifndef _I2C_H_
#define _I2C_H_

#include "common.h"
#include "cpu/g2553.h"

template <typename USCI>
class I2CBus: public USCI {
    const uint8_t _prescale;

    enum {
        SIG_TX = 1,
        SIG_RX = 2
    };

public:
    I2CBus(uint8_t prescale)
        : _prescale(prescale) {
    }

    void init();

public:
    // Begin a write transaction and write the first byte.
    bool start_write(uint8_t addr, uint8_t data);

    // Write additional bytes
    bool write(uint8_t data);

    // Done writing
    void write_done();

    // Check if bus is busy (has ongoing transaction).
    uint8_t busy() {
        return USCI.STAT & USCI.UBUSY;
    }

    // Start transaction.
    bool start_read(uint8_t addr, uint8_t& val);

    // Read byte
    bool read(uint8_t& data);

    // Done reading
    void read_done();

private:
    // Wait for TXBUF ready.  Returns false on timeout or other error.
    bool wait_tx();
    
    // Reset bus/master
    void bus_reset();

private:
    I2CBus(const I2CBus&);
    I2CBus& operator=(const I2CBus&);
};


// There's only a single bus, on UCB0, so use a global.
extern class I2CBus<UCB0> _i2c_bus_master;

// I2C device.
template <typename USCI>
class I2CDevice: public USCI {
public:
    enum State {
        UNATTACHED = 0,     // Device hasn't responded to probe
        PROBING,            // Device is being probed
        ATTACHED            // Device has been successfully probed and is attached
    };

private:
    const uint8_t _addr;
    State _state;

public:
    // Single bus constructor uses global _i2c_bus_master.
    I2CDevice(I2CBus<USCI>&, uint8_t slave_addr)
        : _addr(slave_addr),
          _state(UNATTACHED) {
    }

    // Current state
    State state() const { return _state; }

    // Start/end probe cycle
    void start_probe() { _state = PROBING; }
    void end_probe(bool success) { _state = success ? ATTACHED : UNATTACHED; }

    // Dummy probe to assume device is connected
    void dummy_probe() {
        if (state() != UNATTACHED)
            return;

        start_probe();
        end_probe(true);
    }

    // Begin a write transaction and write the first byte.
    bool start_write(uint8_t data) { 
        if (_state != UNATTACHED) {
            if (_i2c_bus_master.start_write(_addr, data))
                return true;

            _state = UNATTACHED;
        }
        return false;
    }

    // Write additional bytes
    bool write(uint8_t data) {
        if (_state != UNATTACHED) {
            return _i2c_bus_master.write(data);
        }
        return false;
    }

    // Done writing
    void write_done() {
        if (_state != UNATTACHED) {
            _i2c_bus_master.write_done();
        }
    }

    // Write byte array
    void write_bytes(const uint8_t *data, size_t len);

    bool start_read(uint8_t& data) {
        if (_state != UNATTACHED) {
            if (_i2c_bus_master.start_read(_addr, data)) {
                return true;
            }
            _state = UNATTACHED;
        }
        return false;
    }

    // Read byte
    bool read(uint8_t& data);

    // Read block.  Len should be the max length and is updated to reflect the number read.
    void read_bytes(uint8_t* data, size_t& len);

    // Read done, stop
    void read_done() {
        if (_state != UNATTACHED) {
            _i2c_bus_master.read_done();
        }
    }

    // Convenience function to send one or two bytes
    void transmit(uint8_t byte1, uint16_t byte2 = 0x100);

private:
    I2CDevice(const I2CDevice&);
    I2CDevice& operator=(const I2CDevice&);
};

#endif // _I2C_H_
