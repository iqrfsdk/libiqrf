/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: Spi.h
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.org>
 * Date: 2025-05-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "IConnector.h"

namespace iqrf::connector {

/**
 * SPI Connector provides an access to TR module connected via SPI.
 */
class Spi : public IConnector {
 public:
    /**
     * Constructs the SPI connector and initializes the SPI lines.
     */
    Spi();

    /**
     * Release the SPI lines.
     */
    virtual ~Spi();

    // Basic state

    /**
     * Get the current state of the connector.
     *
     * NOTE: clibspi - very different states with more detail
     */
    State getState() const override;

    // Basic communication

    /**
     * Send the data message via SPI.
     */
    void send(const std::vector<uint8_t> data) override;

    /**
     * Read the data synchronously from SPI.
     */
    std::vector<uint8_t> receive() override;

    /**
     * Register the responseHandler for received messages.
     */
    virtual void registerResponseHandler(
        ResponseHandler responseHandler,
        AccessType access
    ) override;
};

}  // namespace iqrf::connector
