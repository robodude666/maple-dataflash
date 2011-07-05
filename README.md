Atmel DataFlash for Maple
=========================

About
-----

This is a fork of the BlockoS's [arduino-dataflash](https://github.com/BlockoS/arduino-dataflash) repo
that aims to be a high-performance LeafLabs Maple library.

This library is intended for libmaple use, but MapleIDE support and examples will be added in the future.

Benchmarks
----------

The below benchmarks were computed using the `main-Benchmark.cpp` application. They are only rough benchmarks
as the tests are not very solid.

The benchmarks were ran using 528-byte buffers, and include 16 pages of read/writes. The SPI peripheral was
configured for a 18 MHz clock speed, MSB First, Mode 0.

The unit `uS` represents microseconds. The unit `Bps` represents bytes per second.

Benchmark 1 - Write via Buffer:

    Time: 228,493 uS.
    Wrote: 8,448 bytes.
    Write Speed: 36,972.69 Bps.

Benchmark 2 - Read via Buffer:

    Time: 32,337 uS.
    Read: 8,448 bytes.
    Errors: 0 errors.
    Read Speed: 261,248.72 Bps.

Benchmark 3 - Read via Memory Page:

    Time: 29,868 uS.
    Read: 8,448 bytes.
    Errors: 0 errors.
    Read Speed: 282,844.52 Bps.

Benchmark 4 - Read via Continuous Array:

    Time: 2,9217 uS.
    Read: 8,448 bytes.
    Errors: 0 errors.
    Read Speed: 289,146.73 Bps.

Benchmark 5 - Read via Continuous Array with DMA:

    Time: 3,544 uS.
    Read: 8,448 bytes.
    Read Speed: 2,383,747.18 Bps.


Notes
-----

### Makefile

I use an edited version of the libmaple Makefile. Below are some things to note:

* `LIB_MAPLE_HOME` is defined in the Makefile. This is because I have multiple libmaple instances installed.
Remove it from the Makefile, or edit to your location.

* `TARGET_MAIN` is defined in `build-targets.mk` and is the current "main.cpp" to be compiled.
