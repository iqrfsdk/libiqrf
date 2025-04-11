/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: main.cpp
 * Authors: Karel Hanák <karel.hanak@iqrf.com>
 * Date: 2024-09-29
 *
 * This file is a part of the LIBIQRF. For the full license information, see the LICENSE file in the project root.
 */

#include "iqrf/gpio/GpioResolver.h"

int main() {
    const iqrf::gpio::GpioResolver *resolver = iqrf::gpio::GpioResolver::GetResolver();
    resolver->dump();

    return 0;
}
