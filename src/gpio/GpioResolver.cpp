#include "iqrf/gpio/GpioResolver.h"

namespace iqrf::gpio {

	GpioResolver * GpioResolver::gpioResolverInstance{nullptr};
	std::mutex GpioResolver::gpioResolverMtx;

	GpioResolver::GpioResolver() {
		gpioMap = getGpioMap();
	}

	GpioResolver::GpioResolver(GpioMap map): gpioMap(map) {
	}

	GpioResolver::~GpioResolver() {
		delete gpioResolverInstance;
	}

	GpioResolver *GpioResolver::GetResolver() {
		std::lock_guard<std::mutex> lock(gpioResolverMtx);
		if (gpioResolverInstance == nullptr) {
			gpioResolverInstance = new GpioResolver();
		}
		return gpioResolverInstance;
	}

	GpioResolver *GpioResolver::GetResolver(GpioMap map) {
		std::lock_guard<std::mutex> lock(gpioResolverMtx);
		if (gpioResolverInstance == nullptr) {
			gpioResolverInstance = new GpioResolver(map);
		}
		return gpioResolverInstance;
	}

	void GpioResolver::resolveGpioPin(int64_t pin, ::std::string &chip, ::std::size_t &line) {
		auto record = gpioMap.find(pin);
		if (record == gpioMap.end()) {
			throw std::runtime_error("No chip and line found for pin no. " + std::to_string(pin));
		}
		auto pair = record->second;
		chip = *pair.first;
		line = pair.second;
	}

	void GpioResolver::dump() {
		for (auto &record : gpioMap) {
			std::cout << *record.second.first << " - line " << record.second.second << " (pin " << record.first << ')' << std::endl;
		}
	}

}
