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

} // namespace dosrecomp::runtime

