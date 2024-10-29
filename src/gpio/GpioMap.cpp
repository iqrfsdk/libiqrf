#include "iqrf/gpio/GpioMap.h"

namespace iqrf::gpio {

	GpioMap getGpioMap() {
#if LIBGPIOD_VERSION_MAJOR == 2
		GpioMap map;
		int64_t npin = 0;
		for (auto const &path : std::filesystem::directory_iterator("/dev")) {
			if (!gpiod::is_gpiochip_device(path)) {
				continue;
			}
			auto chip_info = ::gpiod::chip(path).get_info();
			std::shared_ptr<std::string> chip_name = std::make_shared<std::string>(chip_info.name());
		
			for (size_t line_num = 0; line_num < chip_info.num_lines(); ++line_num) {
				map.insert_or_assign(npin++, std::make_pair(chip_name, line_num));
			}
		}
		return map;
#else
		GpioMap map;
		int64_t npin = 0;
		for (auto &chip: ::gpiod::make_chip_iter()) {
			std::shared_ptr<std::string> chip_name = std::make_shared<std::string>(chip.name());

			for (auto &line: ::gpiod::line_iter(chip)) {
				map.insert_or_assign(npin++, std::make_pair(chip_name, line.offset()));
			}
		}
		return map;
#endif
	}
}
