Atmel DataFlash for Maple
=========================

About
-----

This is a fork of the BlockoS's [arduino-dataflash](https://github.com/BlockoS/arduino-dataflash) repo
that aims to add high-performance LeafLabs Maple support.

This library is intended for libmaple use, but MapleIDE support and examples will be added in the future.

Notes
-----

### Makefile

I use an edited version of the libmaple Makefile. Below are some things to note:

* `LIB_MAPLE_HOME` is defined in the Makefile. This is because I have multiple
libmaple instances installed. Remove it from the Makefile, or edit to your location.

* `TARGET_MAIN` is defined in `build-targets.mk` and is the current "main.cpp" to be compiled.
