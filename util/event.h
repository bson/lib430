// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef _EVENT_H_
#define _EVENT_H_

#include "common.h"
#include "task.h"

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
        while (!_v) {
            if (wait) {
                Task::wait((Task::WChan)this);
            } else  {
                return 0;
            }
        }

        NoInterrupt g;

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
