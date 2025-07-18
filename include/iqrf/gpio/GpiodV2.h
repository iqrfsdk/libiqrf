/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <gpiod.hpp>
#if BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

#include "iqrf/gpio/Common.h"
#include "iqrf/gpio/Base.h"
#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {

/**
 * GPIO driver - gpiod
 */
class Gpiod: public Base {
 public:
    /**
     * Constructor
     * @param config GPIO configuration
     * @throws std::system_error for invalid chip name or busy GPIO line
     * @throws std::out_of_range for line offset out of bands
     */
    explicit Gpiod(const GpioConfig& config);

    /**
     * Destructor
     */
    ~Gpiod() override;

    /**
     * Initializes GPIO line as an input
     */
    void initInput() override;

    /**
     * Initializes GPIO line as an output
     * @param initialValue Initial output value
     */
    void initOutput(bool initialValue) override;

    /**
     * Sets GPIO line direction
     * @param direction GPIO line direction
     */
    void setDirection(iqrf::gpio::GpioDirection direction) override;

    /**
     * Retrieves GPIO line direction
     * @return GPIO line direction
     */
    iqrf::gpio::GpioDirection getDirection() override;

    /**
     * Sets GPIO line output value
     * @param value GPIO line output value
     */
    void setValue(bool value) override;

    /**
     * Retrieves GPIO line input value
     * @return GPIO line input value
     */
    bool getValue() override;

 private:
    /// GPIO chip
    std::unique_ptr<::gpiod::chip> chip;
    /// GPIO line
    ::gpiod::line::offset line;
    /// Request
    std::unique_ptr<::gpiod::line_request> request;
    /// Name
    ::std::string name;
};

}  // namespace iqrf::gpio
