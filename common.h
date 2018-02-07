#ifndef _COMMON_H_
#define _COMMON_H_

#include <msp430.h>

#include <stdint.h>
#include <stddef.h>
#include <strings.h>

typedef unsigned int uint;

// Various compiler stuff; GCC extensions also supported by TI-CC
#define _weak_ __attribute__((weak))
#define _packed_ __attribute__((packed))
#define _used_ __attribute__((used))
#define _noreturn_ __attribute__((noreturn))
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

// RAII scope guard, reentrant
class NoInterruptReent {
    uint16_t _saved;
public:
    NoInterruptReent() {
//        _saved = __get_interrupt_state();
        _saved = __get_SR_register() & GIE;
        __bic_SR_register(GIE);
    }

    ~NoInterruptReent() {
//        __set_interrupt_state(_saved);
        __bis_SR_register(_saved);
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

#define NELEM(A) (sizeof(A) / sizeof(A[0]))

#endif // _COMMON_H_
