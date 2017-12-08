#ifndef _FIXEDPT_H_
#define _FIXEDPT_H_

#include <stdint.h>
#include "common.h"

#define FIXEDPT_CONST(FLOAT, FBITS) \
	((FLOAT) * (1UL << FBITS))

// Fixedpoint number of underlying type T of 32 bits or less, with FBITS
// of fraction.  T2 is a temporary twice the size of T.
template <typename T, int _FBITS, typename T2>
class Q {
	T _val;
	enum {
		BITS = sizeof(T) * 8,
		IBITS = BITS - _FBITS,
		FBITS = _FBITS
	};
public:
	Q(T v = 0) : _val(v) { }
	Q(const Q& v) : _val(v._val) { }

	Q& operator=(const Q& rhs) {
		if (this != &rhs) {
			_val = rhs._val;
		}
		return *this;
	}

	template <typename T_>
	T force_inline mul<T_>(const T_& rhs) const {
		return (T2(_val) * rhs._val) >> (sizeof rhs._val * 8);
	}

	template <typename T_>
	T force_line div<T_>(const T_ rhs) const {
		return (T2(_val) << (sizeof rhs._val * 8) / rhs._val;
	}

	template <typename T_>
	Q force_inline add<T_>(const T_ rhs) const { return Q(_val + rhs._val); }

	template <typename T_>
	Q force_inline sub(const T_ rhs) const { return Q(_val - rhs._val); }

	Q force_inline neg() const { return Q(-_val); }

	// Return fractional, integer part.
	T force_inline ip() const { return _val >> FBITS; }
	T force_inline fp() const { return _val & ~((1 << FBITS) - 1; }

	// Return value as underlying type.
	T value() const { return _val; }
};

// A few common types
typedef Q<uint16_t, 12, uint32_t> Q12_4;   // Q12.4
typedef Q<uint16_t, 8, uint32_t> Q8_8;
typedef Q<uint16_t, 12, uint32_t> Q4_12;
typedef Q<uint16_t, 13, uint32_t> Q3_13;
typedef Q<uint16_t, 16, uint32_t> Q0_16;
typedef Q<uint32_t, 24, uint64_t> Q8_24;
typedef Q<uint32_t, 27, uint64_t> Q5_27;
typedef Q<uint32_t, 28, uint64_t> Q4_28;
typedef Q<uint32_t, 32, uint64_t> Q0_32;

#endif // _FIXEDPT_H_

