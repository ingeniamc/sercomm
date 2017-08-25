# Libsercomm - A Multiplatform Serial Communications Library

[![Build Status](https://travis-ci.org/ingeniamc/sercomm.svg?branch=master)](https://travis-ci.org/ingeniamc/sercomm)
[![Build status](https://ci.appveyor.com/api/projects/status/h0j5rt7sf134cyt3?svg=true)](https://ci.appveyor.com/project/gmarull/sercomm)

`libsercomm` is a portable, pure C implementation library for serial
communications.

## What It Can Do

The library provides:

* access to serial port (r/w)
* serial ports discovery
* serial ports monitor (be notified when a new serial port is plugged or
  unplugged)
* descriptive and detailed error messages

## Building libsercomm

The `libsercomm` library is built using [CMake](<https://cmake.org/>) (version
3.0 or newer) on all platforms.

Under Unix-like systems, `libsercomm` expects `pthreads` to be available (they
should be installed by default). On Linux, `libudev` is also required for device
listing/monitoring support.

On most systems you can build the library using the following commands:

```sh
cmake -H. -Bbuild
cmake --build build
```

### Build options

The following build options are available:

- `WITH_EXAMPLES` (OFF): When enabled, the library usage example applications
  will be built.
- `WITH_DOCS` (OFF): When enabled the API documentation can be built.
- `WITH_ERRDESC` (ON): When enabled, error details description can be obtained.
- `WITH_GITINFO` (OFF): When enabled, the current Git commit hash will be
  included in version. This may be useful to trace installed development builds.
- `WITH_DEVMON` (ON): When enabled, device listing and monitoring will be
  supported.
- `WITH_PIC` (OFF): When enabled, generated code will be position independent.
  This may be useful if you want to embed sercomm into a dynamic library.

Furthermore, *standard* CMake build options can be used. You may find useful to
read this list of [useful CMake variables][cmakeuseful].

[cmakeuseful]: https://cmake.org/Wiki/CMake_Useful_Variables

## Standards Compliance

`libsercomm` is written in [ANSI C][ansic] (C99).

[ansic]: http://en.wikipedia.org/wiki/ANSI_C
