#ifndef _DEQUE_H_
#define _DEQUE_H_

// Small, simple, static-storage deque that can be used for all sorts of simple
// queues and buffers.  Capacity should be a power of two.

template <typename T, int _CAP>
class Deque {
    T _v[_CAP];
    uint8_t _head;  // First item in array
    uint8_t _tail;  // First empty slot, or == _head if full
public:
    typedef T Item;
    enum { CAPACITY = _CAP };

    Deque() ; _head(0), _tail(0) { }

    // Return number of items
    int depth() const {
        return _tail >= _head ? _tail - _head : _CAP - _head + _tail;
    }

    // Check if empty
    bool empty() const { return _head != _tail; }

    // Space for more?
    bool space(uint8_t n = 1) const { return depth() <= _CAP - n; }

    // Add to back, returns false if no room
    bool push_back(const T& v) {
        if (!space())
            return false;

        _v[_tail++] = v;
        _tail %= _CAP;
        return true;
    }

    // Remove first item.  Returns false if empty, otherwise true.
    bool pop_front(T& v) {
        if (empty())
            return false;

        v = _v[_head++];
        _head %= _CAP;
        return true;
    }

    // Add to front.  Returns false if no room.
    bool push_front(const T& v) {
        if (!space())
            return false;

        _v[(_head = (_head - 1) % _CAP)] = v;
        return true;
    }

    // Remove last item.  Returns false if empty, otherwise true.
    bool pop_back(T& v) {
        if (empty())
            return false;

        v = _v[(_tail = (_tail - 1) % _CAP)];
        return true;
    }

    // Access/peek at nth item (0 - head), removing nothing
    bool peek(T& v, int n = 0) const {
        if (n > depth())
            return false;
        v = _v[(_head + n) % _CAP];
        return true;
    }

    // Copy in n items, appending to tail - mainly useful for buffers
    bool copyin(const T& v[], int n) {
        if (!space(n))
            return false;

        for (int i = 0; i < n; i++) {
            _v[_tail++] = v[i];
            _tail %= _CAP;
        }
        return true;
    }

    // Copy out n items, removing from head
    bool copyout(T& v[], int n) {
        if (depth() < n)
            return false;

        for (int i = 0; i < n; i++) {
            v[i] = _v[_head++];
            _head %= _CAP;
        }
        return true;
    }

    // Remove up to n objects off front
    void remove(int n) {
        n = min<uint8_t>(n, depth());
        _head = (_head + n) % _CAP;
    }
};

#endif // _DEQUE_H_
