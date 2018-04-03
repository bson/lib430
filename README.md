# lib430

Bare metal device library for MSP430.  This is really all a collection
of random code, mainly MSP430F5510.  This is all made available under
an MIT-like license in the hope you'll find it useful.  See the file
LICENSE for details.

This is specifically intended for small, low-power applications, hence
there's only Task support for the base 16-bit MSP430, not the
extended-addressing (20 bit) MSP430X.  I don't think the complexity
added by a 20 bit addressing space in what is a 16 bit processor
(based on the elegant pdp-11, to boot) buys anything useful.  Just use
a suitable 32-bit processor instead, like a Cortex-M0 and keep things
simple and straightforward.

Note that some of the code here can be found in newer versions in
http://github.com/bson/enetcore - especially newer font support and
animproved ssd1963 panel driver.  I will be merging these at some
point so I don't end up with two different versions.

This code is TI CC with its gcc extensions, but can be made to build
on plain old gcc with minimal work.

WARNING!  The USB device code is currently broken.  I know.  The
MSP430 UBM isn't very well documented, has clock domain bugs, and I
don't recommend using the code or the MSP430 UBM hardware unless you
explicitly build around TI's demo code as this is the only way it will
ever work reliably.  I may just delete this code at some point.  TI's
poor documentation (or more specifically, spaghetti hardware) is to a
large extent why I abandoned MSP430 in general.  Just use one of the
many excellent CM0/3/4 SoC's out there from NXP, ST, etc if you need
device USB.

There's no sample here, either.  A USB application built with this,
using the SSD1963 and some limited rendering takes about 30k in debug,
17k optimized.  So not bloated really.

It was mainly used to design a programmable bench power supply (20V
5A) with USBTMC control (IEEE 408.2 SCPI commands, tunneled over USB
class 0xfe) and a touch screen panel UI.  (That code is currently not
in any shape to be released.  It's pretty messy.)  I started that
project on a G2553 but decided I really wanted USB control so went
to the F5510.  (Which, admittedly is a bit overkill.)

Feel free to pilfer useful bits and pieces, or if you want to make
something more of this let me know; I might be able to help.
