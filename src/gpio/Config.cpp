#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {


	GpioConfig::GpioConfig(): chip(""), line(0), pin(-1), consumer_name("libiqrf") {
	}
	
	GpioConfig::GpioConfig( ::std::string chip, unsigned int line, ::std::string consumer_name )
		: chip(chip), line(line), pin(-1), consumer_name(consumer_name) {
	}

	GpioConfig::GpioConfig( int64_t pin, ::std::string consumer_name )
		: chip(""), line(0), pin(pin), consumer_name(consumer_name) {

		auto gpioResolver = GpioResolver::GetResolver();
		gpioResolver->resolveGpioPin(pin, chip, line);
	}
	
	const ::std::string GpioConfig::to_string() const {
		::std::ostringstream ss;
		ss << "Gpiod driver configuration: chip=" << chip << ", line=" << line;
		return ss.str();
	};

}
