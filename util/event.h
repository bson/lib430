// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef _EVENT_H_
#define _EVENT_H_

#include "../common.h"
#include "../task.h"

// Simple event mask pattern:
//   Event<uint16_t> _ev;
//
// Task A:
//    // with interrupts disabled
//    const uint16_t next_event = _ev.get_event(true);
//
// Task B:
//    // with interrupts disabled
//    _ev.post(EVENT1 | EVENT2);
//    Task::broadcast(Task::WChan(&_ev));  // wake any task waiting
//
// get_event() returns the lowest bit set in the event mask.  If the
// mask is e.g. 0b0110 it will return 0b0010 and remove it from the
// mask.  The next time it returns 0b0100 and removes that, and from
// thereon it will return 0, or wait for a nonzero mask if the wait
// flag parameter is supplied.
//
template <typename T>
class Event {
    T _v;
public:
    Event() : _v(0) { }
    ~Event() { }

    // Get event mask
    const T events() const { return _v; }

    // Explicit set mask
    void set(T m) { _v = m; }

    // Get next event, or 0 if none; removes event reuturned from pending set.
    // Only one task can wait; only one will be woken with the others remaining
    // in wait indefinitely (or if explicitly activated).
    T get_event(bool wait = false) {
        if (!_v && !wait)
            return 0;

        while (!_v) {
            Task::wait(Task::WChan(this));
        }

        const T pending = _v & ~(_v - 1);
        _v &= ~pending;
        return pending;
    }

    // Post an event.  Note that this by itself doesn't activate the waiter.
    void post(T ev) {
        _v |= ev;
    }
};


#endif // _EVENT_H_
