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

#include "iqrf/gpio/GpiodV1.h"

namespace iqrf::gpio {

	Gpiod::Gpiod(GpioConfig config) {
		chip = ::gpiod::chip(config.chip);
		if (!chip) {
			throw std::runtime_error("No GPIO chip '" + config.chip + "' found");
		}

		if (config.line_name.empty()) {
			line = chip.get_line(config.line);
		} else {
			line = chip.find_line(config.line_name);
		}
		if (!line) {
			throw std::runtime_error("No line '" 
				+ (config.line_name.empty() ? std::to_string(config.line) : config.line_name) 
				+ "' found at chip '" + config.chip + "'");
		}

		name = config.consumer_name;
	}

	Gpiod::~Gpiod() {
		if (line.is_requested())
			line.release();
	}

	void Gpiod::initInput() {
		// To be safe check whether the line wasn't already initialised
		if (line.is_requested())
			line.release();

		// Request the access to the line
		::gpiod::line_request req_conf;
		req_conf.consumer = name;
		req_conf.request_type = ::gpiod::line_request::DIRECTION_INPUT;
		line.request(req_conf);

		// TODO: Probably redundant		
		this->setDirection(GpioDirection::Input);
	}
	
	void Gpiod::initOutput(bool initialValue) {
		// To be safe check whether the line wasn't already initialised
		if (line.is_requested())
			line.release();

		// Request the access to the line
		::gpiod::line_request req_conf;
		req_conf.consumer = name;
		req_conf.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		line.request(req_conf);

		// TODO: Probably redundant		
		this->setDirection(GpioDirection::Output);
		
		this->setValue(initialValue);
	}

	void Gpiod::setDirection(iqrf::gpio::GpioDirection direction) {
		switch (direction) {
			case GpioDirection::Output:
				line.set_direction_output();
				break;
			case GpioDirection::Input:
				line.set_direction_input();
				break;
			default:
				throw ::std::invalid_argument("Unknown direction");
				break;
		}
	}
		
	iqrf::gpio::GpioDirection Gpiod::getDirection() {
		int direction = line.direction();
		switch (direction) {
			case ::gpiod::line::DIRECTION_OUTPUT:
				return iqrf::gpio::GpioDirection::Output;
			case ::gpiod::line::DIRECTION_INPUT:
				return iqrf::gpio::GpioDirection::Input;
			default:
				throw ::std::out_of_range("Unknown direction");
		}
	}

	void Gpiod::setValue(bool value) {
		line.set_value(value);
	}

	bool Gpiod::getValue() {
		return line.get_value();
	}
}
