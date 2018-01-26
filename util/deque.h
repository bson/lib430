#ifndef _DEQUE_H_
#define _DEQUE_H_

// Small, simple, static-storage deque that can be used for all sorts of simple
// queues and buffers.  Capacity should be a power of two.
// The operations are UNSAFE.  Make sure there's space and data available, or you
// will get unpredictable results.

template <typename T, int _CAP>
class Deque {
    T _v[_CAP];
    uint8_t _head;  // First item in array
    uint8_t _tail;  // First empty slot, or == _head if full
public:
    typedef T Item;
    enum { CAPACITY = _CAP };

    Deque() : _head(0), _tail(0) { ; }

    // Reset
    void clear() {
        _head = _tail = 0;
    }

    // Return number of items
    int depth() const {
        return _tail >= _head ? _tail - _head : _CAP - _head + _tail;
    }

    // Check if empty
    bool empty() const { return _head != _tail; }

    // Space for more?
    bool space(uint8_t n = 1) const { return depth() <= _CAP - n; }

    // Add to back
    void push_back(const T& v) {
        _v[_tail++] = v;
        _tail %= _CAP;
    }

    // Remove and return first item.
    const T& pop_front() {
        const T& v = _v[_head++];
        _head %= _CAP;
        return v;
    }

    // Add to front.
    void push_front(const T& v) {
        _v[(_head = (_head - 1) % _CAP)] = v;
    }

    // Remove last item and reutrn.
    const T& pop_back() {
        return _v[(_tail = (_tail - 1) % _CAP)];
    }

    // Access/peek at nth item (0 - head), removing nothing
    const T& peek(int n = 0) const {
        return _v[(_head + n) % _CAP];
    }

    // Add n items, appending to tail - mainly useful for buffers
    void append(const T v[], int n) {
        for (int i = 0; i < n; i++) {
            _v[_tail++] = v[i];
            _tail %= _CAP;
        }
    }

    // Pull out n items, removing from head
    void pull(T v[], int n) {
        for (int i = 0; i < n; i++) {
            v[i] = _v[_head++];
            _head %= _CAP;
        }
    }

    // Remove n objects off front
    void drop(int n) {
        _head = (_head + n) % _CAP;
    }
};

#endif // _DEQUE_H_
