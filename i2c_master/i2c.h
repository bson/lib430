#ifndef _I2C_H_
#define _I2C_H_

#include <msp430.h>
#include <stdint.h>
#include <stddef.h>

// There's only a single bus, so use a global.  I2CBus is merely
// a namespace since all methods are static in this implementation.
extern class I2CBus _i2c_bus_master;

// Make USCI B0 look like a generic bus.  This is really just a
// namespace, so just a bunch of static inline functions.  The global
// however WILL have a footprint of 1 byte so it has a unique address.
// TODO: make sure no vtable is emitted.
class I2CBus {
    const uint8_t _prescale;

    friend class I2CDevice;

    enum {
        SIG_TX = 1,
        SIG_RX = 2
    };

public:
    I2CBus(uint8_t prescale)
        : _prescale(prescale) {
    }

    void init();

protected:
    // Begin a write transaction and write the first byte.
    bool start_write(uint8_t addr, uint8_t data);

    // Write additional bytes
    bool write(uint8_t data);

    // Done writing
    void write_done();

    // Check if bus is busy (has ongoing transaction).
    uint8_t busy() {
        return UCB0STAT & UCBBUSY;
    }

    // Start transaction.
    void start(uint8_t slave, bool read);

    // Send byte
    void send(uint8_t v);    

    // Receive byte
    // This is a placeholder... no use case currently for tx functionality.
    uint8_t recv();

    // End bus transaction
    void stop();

    void wait(uint8_t mask);

private:
    // Wait for TXBUF ready.  Returns false on timeout or other error.
    bool wait_tx();
    
    // Reset bus/master
    void bus_reset();

private:
    I2CBus(const I2CBus&);
    I2CBus& operator=(const I2CBus&);
};


// I2C device.
class I2CDevice {
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
    I2CDevice(I2CBus&, uint8_t slave_addr)
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

    void start_read() { _i2c_bus_master.start(_addr, true); }

    // Convenience function to send one or two bytes
    void transmit(uint8_t byte1, uint16_t byte2 = 0x100);

    // XXX stuff below goes away when read is fixed up

    void stop() { _i2c_bus_master.stop(); }
    void send_byte(uint8_t byte) { _i2c_bus_master.send(byte); }
    uint8_t recv_byte() { return _i2c_bus_master.recv(); }
    bool busy() { return _i2c_bus_master.busy() != 0; }
    void wait_for_idle();

private:
    I2CDevice(const I2CDevice&);
    I2CDevice& operator=(const I2CDevice&);
};

#endif // _I2C_H_
