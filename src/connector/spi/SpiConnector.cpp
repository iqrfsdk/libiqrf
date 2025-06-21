/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiConnector.cpp
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.org>
 * Date: 2025-05-09
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/spi/SpiConnector.h"
#include "iqrf/log/Logging.h"

namespace iqrf::connector::spi {

SpiConnector::SpiConnector(SpiConfig config): IConnector(), config(std::move(config)) {
    this->initGpio();
}

SpiConnector::~SpiConnector() {}



void SpiConnector::initGpio() {
    if (this->config.pgmSwitchGpio) {
        this->config.pgmSwitchGpio->initOutput(false);
    }
    if (this->config.powerEnableGpio) {
        this->config.powerEnableGpio->initOutput(true);
    }
    this->toggleBus(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (this->config.trModuleReset) {
        this->resetTr();
    }

    this->toggleBus(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void SpiConnector::resetTr() {
    if (!this->config.powerEnableGpio.has_value()) {
        return;
    }
    this->config.powerEnableGpio->setValue(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    this->config.powerEnableGpio->setValue(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void SpiConnector::toggleBus(bool enable) {
    if (this->config.busEnableGpio) {
        this->config.busEnableGpio->setValue(enable);
    } else {
        if (this->config.i2cEnableGpio) {
            this->config.i2cEnableGpio->setValue(false);
        }
        if (this->config.spiEnableGpio) {
            this->config.spiEnableGpio->setValue(enable);
        }
        if (this->config.uartEnableGpio) {
            this->config.uartEnableGpio->setValue(false);
        }
    }
}

State SpiConnector::getState() const { return State::NotReady; }

void send(const std::vector<uint8_t> data) {};

std::vector<uint8_t> receive() { return std::vector<uint8_t>(); };

void registerResponseHandler(
    ResponseHandler responseHandler,
    AccessType access
) {};

}
