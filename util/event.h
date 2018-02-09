#ifndef _EVENT_H_
#define _EVENT_H_

#include "common.h"
#include "task.h"

template <typename T>
class Event {
    T _v;
    Task* _waiter;
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
    uint32_t get_event(bool wait = false) {
        while (!_v) {
            if (wait) {
                _waiter = Task::self();
                Task::wait();
                _waiter = NULL;
            } else  {
                return 0;
            }
        }

        NoInterrupt g;

        const uint32_t pending = _v & ~(_v - 1);
        _v &= ~pending;
        return pending;
    }

    // Post an event.  Note that this by itself doesn't activate the waiter.
    void post(T ev) {
        _v |= ev;
    }

    // Wake the waiter, if there is one and there are any events
    void wake() {
        NoInterruptReent g;

        if (_waiter && _v) {
            Task* t = _waiter;
            _waiter = NULL;
            Task::activate(*t);
        }
    }
};


#endif // _EVENT_H_
