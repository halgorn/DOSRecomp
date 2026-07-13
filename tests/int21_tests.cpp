#include "dosrecomp/runtime/int21.hpp"

#include <cstdlib>
#include <iostream>

namespace { std::byte b(unsigned value) { return static_cast<std::byte>(value); } }

int main() {
    dosrecomp::runtime::dos_process_state state;
    const std::vector<std::byte> memory{b('H'), b('i'), b('$')};
    const auto text = dosrecomp::runtime::int21_dispatcher::dispatch({.ah = 0x09, .string_offset = 0}, memory, state);
    const auto character = dosrecomp::runtime::int21_dispatcher::dispatch({.ah = 0x02, .dl = '!'}, memory, state);
    const auto exit = dosrecomp::runtime::int21_dispatcher::dispatch({.ah = 0x4c, .al = 7}, memory, state);
    const auto bad = dosrecomp::runtime::int21_dispatcher::dispatch({.ah = 0x09, .string_offset = 3}, memory, state);
    if (!text || !character || !exit || bad || state.console_output != "Hi!" || state.exit_code != 7) {
        std::cerr << "failed INT 21h dispatch\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

