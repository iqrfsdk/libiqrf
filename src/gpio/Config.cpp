#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {


	GpioConfig::GpioConfig(): chip(""), line(0), pin(-1) {
	}
	
	GpioConfig::GpioConfig( ::std::string chip, unsigned int line ): chip(chip), line(line), pin(-1) {
	}

	GpioConfig::GpioConfig( int64_t pin ): chip(""), line(0), pin(pin) {
		auto gpioResolver = GpioResolver::GetResolver();
		gpioResolver->resolveGpioPin(pin, chip, line);
	}
	
	const ::std::string GpioConfig::to_string() const {
		::std::ostringstream ss;
		ss << "Gpiod driver configuration: chip=" << chip << ", line=" << line;
		return ss.str();
	};

}
