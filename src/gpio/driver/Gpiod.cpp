/**
 * Copyright 2023 MICRORISC s.r.o.
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

#include "iqrf/gpio/driver/Gpiod.h"

namespace iqrf::gpio::driver {

	GpiodConfig::GpiodConfig(): chip(""), line(0) {
	}
	
	GpiodConfig::GpiodConfig( ::std::string chip, unsigned int line ): chip(chip), line(line) {
	}
	
	const ::std::string GpiodConfig::to_string() const {
		::std::ostringstream ss;
		ss << "Gpiod driver configuration: chip=" << chip << ", line=" << line;
		return ss.str();
	};


	Gpiod::Gpiod(GpiodConfig config) {
		chip = ::gpiod::chip(config.chip);
		line = chip.get_line(config.line);
	}

	Gpiod::~Gpiod() {
		this->destroy();
	}

	void Gpiod::initInput() {
		// To be safe check whether the line wasn't already initialised
		if (line.is_requested())
			line.release();

		// Create a consumer name out of the chip and line
		::std::stringstream ss;
		ss << chip.name() << "-" << line.offset();

		// Request the access to the line
		::gpiod::line_request req_conf;
		req_conf.consumer = ss.str();
		req_conf.request_type = ::gpiod::line_request::DIRECTION_INPUT;
		line.request(req_conf);

		// TODO: Probably redundant		
		this->setDirection(GpioDirection::Input);
	}
	
	void Gpiod::initOutput(bool initialValue) {
		// To be safe check whether the line wasn't already initialised
		if (line.is_requested())
			line.release();

		// Create a consumer name out of the chip and line
		::std::stringstream ss;
		ss << chip.name() << "-" << line.offset();

		// Request the access to the line
		::gpiod::line_request req_conf;
		req_conf.consumer = ss.str();
		req_conf.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		line.request(req_conf);

		// TODO: Probably redundant		
		this->setDirection(GpioDirection::Output);
		
		this->setValue(initialValue);
	}

	void Gpiod::destroy() {
		if (line.is_requested())
			line.release();
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
				break;
			case ::gpiod::line::DIRECTION_INPUT:
				return iqrf::gpio::GpioDirection::Output;
				break;
			default:
				throw ::std::out_of_range("Unknown direction");
				break;
		}
	}

	void Gpiod::setValue(bool value) {
		line.set_value(value);
	}

	bool Gpiod::getValue() {
		return line.get_value();
	}
}
