#include <set>

#include "iqrf/gpio/GpioMap.h"

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
		std::set<int> test;
		std::set<std::filesystem::path> chips;

		for (auto const &entry: std::filesystem::directory_iterator{"/dev"}) {
			if (gpiod::is_gpiochip_device(entry.path())) {
				chips.insert(entry.path());
			}
		}

		for (auto const &chip_path: chips) {
			auto chip_info = ::gpiod::chip(chip_path).get_info();
			std::shared_ptr<std::string> chip_name = std::make_shared<std::string>(chip_info.name());

			for (std::size_t line = 0; line < chip_info.num_lines(); ++line) {
				map.insert_or_assign(npin++, std::make_pair(chip_name, line));
			}
		}
#endif

		return map;
	}
}
