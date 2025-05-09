/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: Spi.cpp
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.org>
 * Date: 2025-05-09
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/Spi.h"

namespace iqrf::connector {

Spi::Spi() {}

Spi::~Spi() {}

State Spi::getState() const { return State::NotReady; }

void send(const std::vector<uint8_t> data) {};

std::vector<uint8_t> receive() { return std::vector<uint8_t>(); };

void registerResponseHandler(
    ResponseHandler responseHandler,
    AccessType access
) {};

}
