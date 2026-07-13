#include "dosrecomp/runtime/int16.hpp"

namespace dosrecomp::runtime {
std::expected<int16_result, keyboard_error>
int16_dispatcher::dispatch(const int16_request& request, std::deque<keyboard_key>& queue) {
    if (request.ah == 0x00) {
        if (queue.empty()) return std::unexpected(keyboard_error{"INT 16h/00 keyboard queue is empty"});
        const auto key = queue.front();
        queue.pop_front();
        return int16_result{.key = key, .available = true};
    }
    if (request.ah == 0x01) {
        if (queue.empty()) return int16_result{.key = std::nullopt, .available = false};
        return int16_result{.key = queue.front(), .available = true};
    }
    return std::unexpected(keyboard_error{"unsupported INT 16h function"});
}

} // namespace dosrecomp::runtime

