#ifndef _ACCESSORS_H_
#define _ACCESSORS_H_

// This file contains accessor names.  The reason for this file
// is that we collect control registers into uniform namespaces
// in the form of classes.
// For example,
//   template <volatile uint8_t& _CTL0>
//   class UCA {
//      ...
//   };
// The idea here is that UCA.CTL0 is used instead of the actual
// control register, but the template parameter (_CTL0) isn't public
// and doesn't exist outside the declaration and function definitions,
// so it's not possible to access it directly.  With a type we could
// use a C++11 type alias that's public.  With an scalar we could use
// an enum.  But with references, functions, and some other types this
// isn't possible.
// We could use:
//    volatile uint8_t& CTL0;
//    UCA() : CTL0(_CTL0) { }
// This works, but adds memory footprint wherever we want to derive
// something from this namespace.
// So we use an accessor of the form:
//    typeof _CTL0 CTL0() { return _CTL0; }
// However, it's kind of awkward and not particularly pleasant to
// use:
//    UCA.CTL0() = 0xff;
// The code using CTL0 shouldn't have to know it's a function.  So
// what we actually do is:
//    typeof _CTL0 getCTL0() { return _CTL0; }
//    #define CTL0 getCTL0()
// But, since #define's are global in scope they can only be added
// once, in a global scope.  Hence they are collected in this file.

#define force_inline inline __attribute__((always_inline))

#define ACCESSOR(TYPE, FUNC, TPARM) \
    static force_inline TYPE FUNC() { return TPARM; }
    
#define ABCTL getABCTL()
#define BR0 getBR0()
#define BR1 getBR1()
#define CTL getCTL()
#define CTL0 getCTL0()
#define CTL1 getCTL1()
#define I2CIE getI2CIE()
#define I2COA getI2COA()
#define I2CSA getI2CSA()
#define IE2 getIE2()
#define IFG getIFG()
#define IG2 getIFG2()
#define IRRCTL getIRRCTL()
#define IRTCTL getIRTCTL()
#define MCTL getMCTL()
#define RXBUF getRXBUF()
#define STAT getSTAT()
#define TA_CCTL0 getCCTL0()
#define TA_CCR0 getCCR0()
#define TA_CCTL1 getCCTL1()
#define TA_CCR1 getCCR1()
#define TA_CCTL2 getCCTL2()
#define TA_CCR2 getCCR2()
#define TA_IV getIV()
#define TA_R getR()
#define TXBUF getTXBUF()

#endif // _ACCESSORS_H_
