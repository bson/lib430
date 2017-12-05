#ifndef _I2C_H_
#define _I2C_H_

#include "common.h"
#include "cpu/g2553.h"

template <typename USCI, uint32_t _SPEED>
class I2CBus {
public:
    static void init();

public:
    enum { PRESCALE = SMCLK/_SPEED };

    // Begin a write transaction and write the first byte.
    static bool start_write(uint8_t addr, uint8_t data);

    // Write additional bytes
    static bool write(uint8_t data);

    // Done writing
    static void write_done();

    // Check if bus is busy (has ongoing transaction).
    static uint8_t busy() {
        return USCI::STAT & USCI::UBUSY;
    }

    // Start transaction.
    static bool start_read(uint8_t addr, uint8_t* data);

    // Read byte
    static bool read(uint8_t* data);

    // Done reading
    static void read_done();

private:
    // Wait for TXBUF ready.  Returns false on timeout or other error.
    static bool wait_tx();
    
    // Reset bus/master
    static void bus_reset();

private:
    I2CBus(const I2CBus&);
    I2CBus& operator=(const I2CBus&);
};


// I2C device.
template <typename _Bus, typename USCI>
class I2CDevice {
public:
    enum State {
        UNATTACHED = 0,     // Device hasn't responded to probe
        PROBING,            // Device is being probed
        ATTACHED            // Device has been successfully probed and is attached
    };

private:
    uint8_t _addr;
    State _state;

public:
    typedef _Bus Bus;

    // Single bus constructor uses global _i2c_bus_master.
    I2CDevice(uint8_t slave_addr)
        : _addr(slave_addr),
          _state(UNATTACHED) {
    }

    // Current state
    State state() const { return _state; }

    // Start/end probe cycle
    void start_probe() { _state = PROBING; }
    void end_probe(bool success) { _state = (success ? ATTACHED : UNATTACHED); }

    // Dummy probe to assume device is connected
    void dummy_probe() {
        _state = ATTACHED;
    }

    // Begin a write transaction and write the first byte.
    bool start_write(uint8_t data) { 
        if (_state != UNATTACHED) {
            if (Bus::start_write(_addr, data))
                return true;

            _state = UNATTACHED;
        }
        return false;
    }

    // Write additional bytes
    bool write(uint8_t data) {
        if (_state != UNATTACHED) {
            return Bus::write(data);
        }
        return false;
    }

    // Done writing
    void write_done() {
        if (_state != UNATTACHED) {
            Bus::write_done();
        }
    }

    // Write byte array
    void write_bytes(const uint8_t *data, size_t len);

    bool start_read(uint8_t* data) {
        if (_state != UNATTACHED) {
            if (Bus::start_read(_addr, data)) {
                return true;
            }
            _state = UNATTACHED;
        }
        return false;
    }

    // Read byte
    bool read(uint8_t* data) {
    		if (_state != UNATTACHED) {
    			return Bus::read(data);
    		}
    		return false;
    }

    // Read block.  Len should be the max length and is updated to reflect the number read.
    void read_bytes(uint8_t* data, size_t& len);

    // Read done, stop
    void read_done() {
        if (_state != UNATTACHED) {
            Bus::read_done();
        }
    }

    // Convenience function to send one or two bytes
    void transmit(uint8_t byte1, uint16_t byte2 = 0x100);

    // For devices that need to use address bits, e.g. 24LCxxx
    void force_inline mask_addr(uint8_t mask, uint8_t value) {
    		_addr = (_addr & ~mask) | (value & mask);
    }

private:
    I2CDevice(const I2CDevice&);
    I2CDevice& operator=(const I2CDevice&);
};

#endif // _I2C_H_
