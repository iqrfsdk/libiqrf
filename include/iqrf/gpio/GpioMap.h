#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include <gpiod.hpp>
#include "iqrf/gpio/version.h"

namespace iqrf::gpio {

	typedef std::map<std::size_t, std::pair<std::shared_ptr<std::string>, size_t>> GpioMap;

	/**
	 * Get map of GPIO pins and chips names / line offsets
	 * @return Map of PGIO pins and chips / line offsets
	 */
	GpioMap getGpioMap();
}
