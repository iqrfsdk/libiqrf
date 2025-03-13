#include <algorithm>
#include <string>
#include <vector>

#include "iqrf/gpio/GpioMap.h"

#define GPIO_DIRECTORY "/dev"
#define GPIO_CHIP_PREFIX "gpiochip"
#define GPIO_CHIP_PREFIX_LEN 8

namespace iqrf::gpio {

	GpioMap getGpioMap() {
		GpioMap map;
		std::size_t npin = 0;

#if LIBGPIOD_VERSION_MAJOR == 1
		for (auto &chip: ::gpiod::make_chip_iter()) {
			std::shared_ptr<std::string> chip_name = std::make_shared<std::string>(chip.name());

			for (auto &line: ::gpiod::line_iter(chip)) {
				map.insert_or_assign(npin++, std::make_pair(chip_name, line.offset()));
			}
		}
#else
		std::vector<std::filesystem::path> chips;

		// Load all GPIO chips
		for (auto const &entry: std::filesystem::directory_iterator{GPIO_DIRECTORY}) {
			if (entry.path().filename().string().find(GPIO_CHIP_PREFIX) == 0) {  // We care only about /dev/gpiochip* entries
				if (gpiod::is_gpiochip_device(entry.path())) {  // Make sure it is GPIO
					chips.emplace_back(entry.path());
				}
			}
		}

		// Sort the chips in numerical order by their ordinal number
		std::sort(chips.begin(), chips.end(), [](const auto &a, const auto &b) {
			try {
				auto a_no = std::stoul(a.filename().string().substr(GPIO_CHIP_PREFIX_LEN));
				auto b_no = std::stoul(b.filename().string().substr(GPIO_CHIP_PREFIX_LEN));
				return a_no < b_no;
			} catch (const std::exception &e) {  // Do not sort if error
				return true;
			}
		});

		// Create the mapping according to the sorted chips
		for (auto const &chip_path: chips) {
			try {
				auto chip_info = ::gpiod::chip(chip_path).get_info();
				std::shared_ptr<std::string> chip_name = std::make_shared<std::string>(chip_info.name());

				for (std::size_t line = 0; line < chip_info.num_lines(); ++line) {
					map.insert_or_assign(npin++, std::make_pair(chip_name, line));
				}
			} catch (const std::exception &e) {}  // Ignore the failed chips
		}
#endif

		return map;
	}
}
