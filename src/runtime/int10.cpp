#include "dosrecomp/runtime/int10.hpp"

namespace dosrecomp::runtime {
std::expected<void, video_error>
int10_dispatcher::dispatch(const int10_request& request, text_video_state& state) {
    if (request.ah == 0x02) {
        state.row = request.dh;
        state.column = request.dl;
        return {};
    }
    if (request.ah == 0x0e) {
        const auto character = static_cast<char>(request.al);
        state.output.push_back(character);
        if (character == '\n' || character == '\r') {
            if (character == '\n') ++state.row;
            state.column = 0;
        } else {
            ++state.column;
        }
        return {};
    }
    return std::unexpected(video_error{"unsupported INT 10h function"});
}

std::expected<void, video_error>
int10_mode_dispatcher::dispatch(const int10_request& request, video_mode_state& state) {
    if (request.ah != 0x00) return std::unexpected(video_error{"unsupported INT 10h mode function"});
    if (request.al == 0x03) {
        state.active = video_mode::text_80x25;
        return {};
    }
    if (request.al == 0x13) {
        state.active = video_mode::mode13h;
        return {};
    }
    return std::unexpected(video_error{"unsupported BIOS video mode"});
}

} // namespace dosrecomp::runtime
