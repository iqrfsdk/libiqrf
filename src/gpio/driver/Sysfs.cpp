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

#include "iqrf/gpio/driver/Sysfs.h"

namespace iqrf::gpio::driver {

	Sysfs::Sysfs(iqrf::gpio::driver::SysfsConfig config): config(config) {
	}

	Sysfs::~Sysfs() {
		this->unexportPin();
	}

	boost::filesystem::path Sysfs::createSysfsPath(SysfsActions action) const {
		std::ostringstream path;
		path << "/sys/class/gpio/";
		switch (action) {
			case SysfsActions::Export:
				path << "export";
				break;
			case SysfsActions::Unexport:
				path << "unexport";
				break;
			case SysfsActions::Direction:
				path << "gpio" << this->config.pin << "/direction";
				break;
			case SysfsActions::Value:
				path << "gpio" << this->config.pin << "/value";
				break;
			case SysfsActions::CheckExport:
				path << "gpio" << this->config.pin;
				break;
			default:
				throw std::runtime_error("Unknown sysfs action.");
		}
		return {path.str()};
	}

	void Sysfs::checkExport() {
		if (!boost::filesystem::exists(this->createSysfsPath(SysfsActions::CheckExport))) {
			throw std::runtime_error("GPIO pin " + std::to_string(this->config.pin) + " is not exported. Reason: " + std::to_string(errno) + " (" + strerror(errno) + ").");
		}
	}

	void Sysfs::initInput() {
		if (boost::filesystem::exists(this->createSysfsPath(SysfsActions::CheckExport)) &&
			this->getDirection() != GpioDirection::Input) {
			throw std::runtime_error("GPIO pin " + std::to_string(this->config.pin) + " is already initialized as output.");
		}
		this->exportPin();
		this->setDirection(GpioDirection::Input);
	}

	void Sysfs::initOutput(bool initialValue) {
		if (boost::filesystem::exists(this->createSysfsPath(SysfsActions::CheckExport)) &&
			this->getDirection() != GpioDirection::Output) {
			throw std::runtime_error("GPIO pin " + std::to_string(this->config.pin) + " is already initialized as input.");
		}
		this->exportPin();
		this->setDirection(GpioDirection::Output);
		this->setValue(initialValue);
	}

	void Sysfs::destroy() {
		this->unexportPin();
	}

	void Sysfs::exportPin() {
		if (boost::filesystem::exists(this->createSysfsPath(SysfsActions::CheckExport))) {
			return;
		}
		boost::filesystem::path path = this->createSysfsPath(SysfsActions::Export);
		boost::filesystem::ofstream file(path, std::ios::out);
		if (!file.is_open()) {
			throw std::runtime_error("Cannot export GPIO pin " + std::to_string(this->config.pin) + ". Reason: " + std::to_string(errno) + " (" + strerror(errno) + ").");
		}
		file << this->config.pin;
		file.close();
	}

	void Sysfs::unexportPin() {
		if (!boost::filesystem::exists(this->createSysfsPath(SysfsActions::CheckExport))) {
			return;
		}
		boost::filesystem::path path = this->createSysfsPath(SysfsActions::Direction);
		boost::filesystem::ofstream file(path, std::ios::out);
		if (!file.is_open()) {
			throw std::runtime_error("Cannot unexport GPIO pin " + std::to_string(this->config.pin) + ". Reason: " + std::to_string(errno) + " (" + strerror(errno) + ").");
		}
		file << this->config.pin;
		file.close();
	}

	iqrf::gpio::GpioDirection Sysfs::getDirection() {
		boost::filesystem::path path = this->createSysfsPath(SysfsActions::Direction);
		this->checkExport();
		boost::filesystem::ifstream file(path, std::ios::in);
		if (!file.is_open()) {
			throw std::runtime_error("Cannot open GPIO pin " + std::to_string(this->config.pin) + " direction file. Reason: " + std::to_string(errno) + " (" + strerror(errno) + ").");
		}
		std::string direction;
		file >> direction;
		file.close();
		if (direction == "in") {
			return GpioDirection::Input;
		} else if (direction == "out") {
			return GpioDirection::Output;
		} else {
			throw std::runtime_error("Unknown GPIO pin " + std::to_string(this->config.pin) + " direction: " + direction + ".");
		}
	}

	void Sysfs::setDirection(iqrf::gpio::GpioDirection direction) {
		boost::filesystem::path path = this->createSysfsPath(SysfsActions::Direction);
		this->checkExport();
		boost::filesystem::ofstream file(path, std::ios::out);
		if (!file.is_open()) {
			throw std::runtime_error("Cannot open GPIO pin " + std::to_string(this->config.pin) + " direction file. Reason: " + std::to_string(errno) + " (" + strerror(errno) + ").");
		}
		file << (direction == GpioDirection::Input ? "in" : "out");
		file.close();
	}

	bool Sysfs::getValue() {
		boost::filesystem::path path = this->createSysfsPath(SysfsActions::Value);
		this->checkExport();
		boost::filesystem::ifstream file(path, std::ios::in);
		if (!file.is_open()) {
			throw std::runtime_error("Cannot open GPIO pin " + std::to_string(this->config.pin) + " value file. Reason: " + std::to_string(errno) + " (" + strerror(errno) + ").");
		}
		std::string value;
		file >> value;
		file.close();
		if (value == "0") {
			return false;
		} else if (value == "1") {
			return true;
		} else {
			throw std::runtime_error("Unknown GPIO pin " + std::to_string(this->config.pin) + " value: " + value + ".");
		}
	}

	void Sysfs::setValue(bool value) {
		boost::filesystem::path path = this->createSysfsPath(SysfsActions::Value);
		this->checkExport();
		boost::filesystem::ofstream file(path, std::ios::out);
		if (!file.is_open()) {
			throw std::runtime_error("Cannot open GPIO pin " + std::to_string(this->config.pin) + " value file. Reason: " + std::to_string(errno) + " (" + strerror(errno) + ").");
		}
		file << (value ? "1" : "0");
		file.close();
	}

}
