#include "dosrecomp/runtime/bios_timer.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    dosrecomp::runtime::bios_timer timer{dosrecomp::runtime::bios_timer::ticks_per_day - 2};
    timer.advance(4);
    const auto first = dosrecomp::runtime::int1a_dispatcher::dispatch({.ah = 0x00}, timer);
    const auto second = dosrecomp::runtime::int1a_dispatcher::dispatch({.ah = 0x00}, timer);
    const auto unsupported = dosrecomp::runtime::int1a_dispatcher::dispatch({.ah = 0x01}, timer);
    if (!first || first->ticks != 2 || first->midnight_rollovers != 1 || !second ||
        second->midnight_rollovers != 0 || unsupported) {
        std::cerr << "failed INT 1Ah timer dispatch\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
