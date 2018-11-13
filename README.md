# lib430

Bare metal device library for MSP430.  This is really all a collection
of random code, mainly MSP430F5510.  This is all made available under
an MIT-like license in the hope you'll find it useful.  See the file
LICENSE for details.

This is specifically intended for small, low-power applications, hence
there's only Task support for the base 16-bit MSP430, not the
extended-addressing (20 bit) MSP430X.  I don't think the complexity
introduced by a 20 bit address space in what is a 16 bit processor
(based on the elegant pdp-11, to boot) buys anything useful.  Just use
a suitable 32-bit processor instead, like a Cortex-M0 and keep things
simple and straightforward.

Note that some of the code here can be found in newer versions in
http://github.com/bson/enetcore - especially newer font support and
animproved ssd1963 panel driver.  I will be merging these at some
point so I don't end up with two different versions.

This code is TI CC with its gcc extensions, but can be made to build
on plain old gcc with minimal work.

Feel free to pilfer useful bits and pieces, or if you want to make
something more of this let me know; I might be able to help.
