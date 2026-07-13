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
    dosrecomp::runtime::conventional_memory conventional(4);
    const auto allocated = dosrecomp::runtime::int21_memory_dispatcher::dispatch({.ah = 0x48, .bx = 2}, conventional);
    const auto released = allocated && allocated->allocated_segment
        ? dosrecomp::runtime::int21_memory_dispatcher::dispatch({.ah = 0x49, .es = *allocated->allocated_segment}, conventional)
        : std::expected<dosrecomp::runtime::int21_memory_result, dosrecomp::runtime::dos_error>{std::unexpected(dosrecomp::runtime::dos_error{"allocation failed"})};
    const dosrecomp::runtime::dos_clock clock{.year = 1993, .month = 5, .day = 1, .weekday = 6, .hour = 12, .minute = 34, .second = 56, .hundredths = 78};
    const auto date = dosrecomp::runtime::int21_clock_dispatcher::dispatch({.ah = 0x2a}, clock);
    const auto time = dosrecomp::runtime::int21_clock_dispatcher::dispatch({.ah = 0x2c}, clock);
    if (!text || !character || !exit || bad || !allocated || !allocated->allocated_segment || !released || !date || !date->value ||
        !time || !time->value || time->value->hour != 12 ||
        state.console_output != "Hi!" || state.exit_code != 7) {
        std::cerr << "failed INT 21h dispatch\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
