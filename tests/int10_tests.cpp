#include "dosrecomp/runtime/int10.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    dosrecomp::runtime::text_video_state state;
    const auto cursor = dosrecomp::runtime::int10_dispatcher::dispatch({.ah = 0x02, .dh = 3, .dl = 5}, state);
    const auto character = dosrecomp::runtime::int10_dispatcher::dispatch({.ah = 0x0e, .al = 'A'}, state);
    const auto newline = dosrecomp::runtime::int10_dispatcher::dispatch({.ah = 0x0e, .al = '\n'}, state);
    const auto unsupported = dosrecomp::runtime::int10_dispatcher::dispatch({.ah = 0x13}, state);
    dosrecomp::runtime::video_mode_state mode;
    const auto mode13 = dosrecomp::runtime::int10_mode_dispatcher::dispatch({.ah = 0x00, .al = 0x13}, mode);
    const auto invalid_mode = dosrecomp::runtime::int10_mode_dispatcher::dispatch({.ah = 0x00, .al = 0x12}, mode);
    if (!cursor || !character || !newline || unsupported || !mode13 || invalid_mode ||
        mode.active != dosrecomp::runtime::video_mode::mode13h || state.output != "A\n" || state.row != 4 || state.column != 0) {
        std::cerr << "failed INT 10h text dispatch\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
