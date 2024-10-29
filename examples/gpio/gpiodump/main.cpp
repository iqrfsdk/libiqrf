#include "iqrf/gpio/GpioResolver.h"

int main() {

	auto resolver = iqrf::gpio::GpioResolver::GetResolver();
	resolver->dump();

	return 0;
}
