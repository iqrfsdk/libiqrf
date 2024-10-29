#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include <gpiod.hpp>
#include "iqrf/gpio/version.h"

namespace iqrf::gpio {

#if LIBGPIOD_VERSION_MAJOR == 2
	typedef std::map<int64_t, std::pair<std::shared_ptr<std::string>, size_t>> GpioMap;
#else
	typedef std::map<int64_t, std::pair<std::shared_ptr<std::string>, unsigned int>> GpioMap;
#endif

	/**
	 * Get map of GPIO pins and chips names / line offsets
	 * @return Map of PGIO pins and chips / line offsets
	 */
	GpioMap getGpioMap();
}
