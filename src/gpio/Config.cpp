#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {


	GpioConfig::GpioConfig(): chip(""), line(0), line_name(""), pin(-1), consumer_name(default_consumer_name) {
		// Caution: Line is 0, which is a valid line number
	}

	GpioConfig::GpioConfig( ::std::string chip, ::std::size_t line, ::std::string consumer_name )
		: chip(chip), line(line), line_name(""), pin(-1), consumer_name(consumer_name) {
	}

	GpioConfig::GpioConfig(::std::string chip, ::std::string line_name, ::std::string consumer_name)
		: chip(chip), line(0), line_name(line_name), pin(-1), consumer_name(consumer_name) {
	}

	GpioConfig::GpioConfig( int64_t pin, ::std::string consumer_name )
		: chip(""), line(0), line_name(""), pin(pin), consumer_name(consumer_name) {

		auto gpioResolver = GpioResolver::GetResolver();
		gpioResolver->resolveGpioPin(pin, chip, line);
	}

	const ::std::string GpioConfig::to_string() const {
		::std::ostringstream ss;
		ss << "Gpiod driver configuration: chip=" << chip << ", line=" << line;
		return ss.str();
	};

}
