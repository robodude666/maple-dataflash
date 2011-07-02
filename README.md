Atmel DataFlash for Maple
=========================

About
-----

This is a fork of the BlockoS's [arduino-dataflash](https://github.com/BlockoS/arduino-dataflash) repo
that aims to add high-performance LeafLabs Maple support.

This library is intended for libmaple use, but MapleIDE support and examples will be added in the future.

Benchmarks
----------

The below benchmarks were computed using the `main-Benchmark.cpp` application. They are only rough benchmarks
as the tests are not very solid.

The benchmarks were ran using 528-byte buffers, and 16 pages. The SPI peripheral was configured for a
18MHz clock speed, MSB First, Mode 0.

The unit `uS` represents microseconds. The unit `Bps` represents bytes per second.

Benchmark 1 - Write via Buffer:

    Time: 224956 uS.
    Wrote: 8448 bytes.
    Write Speed: 37554.01 Bps.

Benchmark 2 - Read via Buffer:

    Time: 32802 uS.
    Read: 8448 bytes.
    Errors: 0 errors.
    Read Speed: 257545.27 Bps.

Benchmark 3 - Read via Memory Page:

    Time: 29877 uS.
    Read: 8448 bytes.
    Errors: 0 errors.
    Read Speed: 282759.31 Bps.

Benchmark 4 - Read via Continuous Array:

    Time: 29453 uS.
    Read: 8448 bytes.
    Errors: 0 errors.
    Read Speed: 286829.86 Bps.

Notes
-----

### Makefile

I use an edited version of the libmaple Makefile. Below are some things to note:

* `LIB_MAPLE_HOME` is defined in the Makefile. This is because I have multiple libmaple instances installed.
Remove it from the Makefile, or edit to your location.

* `TARGET_MAIN` is defined in `build-targets.mk` and is the current "main.cpp" to be compiled.
