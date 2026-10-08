/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: ISpiDevice.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/spi/ISpiDevice.h"

#include <memory>
#include <stdexcept>

#ifdef __linux__
#include "iqrf/connector/spi/LinuxSpiDevice.h"
#endif

namespace iqrf::connector::spi {

std::unique_ptr<ISpiDevice> createSpiDevice(const SpiDeviceConfig &config) {
#ifdef __linux__
    return std::make_unique<LinuxSpiDevice>(config);
#else
    // TODO: FreeBSD - implement spigen(4) based device (SPIGENIOC_TRANSFER)
    (void) config;
    throw std::runtime_error("SPI connector is not supported on this platform");
#endif
}

}  // namespace iqrf::connector::spi
