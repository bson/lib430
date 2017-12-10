#ifndef _COMMON_H_
#define _COMMON_H_

namespace msp430 {
#include <msp430.h>
};

#include <stdint.h>
#include <stddef.h>

typedef unsigned int uint;

// Various compiler stuff; GCC extensions also supported by TI-CC
#define _weak_ __attribute__((weak))
#define _ro_ __attribute__((section ".rodata"))
#define _packed_ __attribute__((packed))
#define _intr_(VEC) __interrupt __attribute__((interrupt(VEC)))


// These two wrap the MSP430 compiler intrinsics to factor it out as a compiler
// dependency.
static inline void
enable_interrupt() {
    __enable_interrupt();
}

static inline void
disable_interrupt() {
    __disable_interrupt();
}

// RAII scope guard
class NoInterrupt {
public:
    NoInterrupt() {
        disable_interrupt();
    }

    ~NoInterrupt() {
        enable_interrupt();
    }
};

template <typename T>
static inline const
T& max(const T& a, const T& b) {
    return a < b ? b : a;
}

template <typename T>
static inline const
T& min(const T& a, const T& b) {
    return b < a ? b : a;
}

template <typename T>
static inline T swap(T& a, const T& b) {
    T tmp = a;
    a = b;
    return tmp;
}

#endif // _COMMON_H_
