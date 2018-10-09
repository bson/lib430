#ifndef _IOQUEUE_H_
#define _IOQUEUE_H_

#include <stdint.h>

// This is a very simple I/O buffer queue consisting of a number of buffers,
// a ring head, and a ring tail.  To eliminate ambiguity
// over whether the ring is emtpy or full (head == tail), it explicitly
// tracks the number of items.  The underlying objects are owned by the queue.
// Its intended purpose is for producer-consumer patterns.  It's task safe.
//
// Usage goes like this:
// Receiver task:
//   recv_buf = queue.head()
//   fill_buffer(recv_buf)
//   queue.process_head()
//
// Processing task:
//   proc_buf = queue.tail()
//   process(proc_buf)
//   queue.recycle_tail(proc_buf)


template <uint16_t _SIZE, uint16_t _DEPTH>
class IOQueue {
public:
    enum {
        SIZE = _SIZE,
        DEPTH = _DEPTH,
        STORAGE = SIZE * DEPTH,
    };

private:
    uint16_t _head;
    uint16_t _tail;
    uint16_t _depth;
    uint8_t _v[STORAGE];

public:
    IOQueue()
    : _depth(0), _head(0), _tail(0) {
    }

    // Reset
    void clear() {
        _depth = _head = _tail = 0;
    }

    // Check if there is another buffer available to fill at head
    bool head_available() { return _depth != STORAGE; }

    // Check if there is a buffer available for processing at tail
    bool tail_available() { return _depth != 0; }

    // Return head buffer, to fill
    uint8_t *head() { return _v + _head; }

    // Release head buffer for processing
    void process_head() {
        _depth += SIZE;
        if ((_head += SIZE) >= STORAGE)
            _head -= STORAGE;
    }

    // Return tail buffer for processing
    uint8_t *tail() { return _v + _tail; }

    // Return tail buffer to pool
    void recycle_tail() {
        _depth -= SIZE;
        if ((_tail += SIZE) >= STORAGE)
            _tail -= STORAGE;
    }

private:
    IOQueue(const IOQueue&);
    IOQueue& operator=(const IOQueue&);
};

#endif // _IOQUEUE_H_
