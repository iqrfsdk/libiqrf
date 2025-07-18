# libiqrf

[![Build Status](https://gitlab.iqrf.org/open-source/iqrf-sdk/libiqrf/badges/master/pipeline.svg)](https://gitlab.iqrf.org/open-source/iqrf-sdk/libiqrf/pipelines)
[![Apache License](https://img.shields.io/badge/license-APACHE2-blue.svg)](LICENSE)

## Description

This repository contains the IQRF libraries for C/C++.

![Library schematics](images/libiqrf.drawio.png "Library schematics")

### Libraries

- [`libiqrf-connector-uart`](src/connector/uart) - UART connector library
- [`libiqrf-gpio`](src/gpio) - GPIO library
- [`libiqrf-log`](src/log) - Logging library

## Dependencies

- [CMake](https://cmake.org/) >= 3.18
- [Boost](https://www.boost.org/) (log, program_options, system)
- [libgpiod](https://libgpiod.readthedocs.io/en/latest/index.html) >= 1.6.2
- [Ccache](https://ccache.dev/) >= 3.7 (optional)

### Testing
- [GTest](https://google.github.io/googletest/)

## Build

```bash
git clone https://gitlab.iqrf.org/open-source/iqrf-sdk/libiqrf.git
cd libiqrf/
mkdir build
cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DBUILD_STATIC=TRUE -DUSE_CCACHE=TRUE
cmake --build build
```

## Test

```bash
cd build/test/
ctest
```

