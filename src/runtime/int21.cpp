#include "dosrecomp/runtime/int21.hpp"

namespace dosrecomp::runtime {
std::expected<void, dos_error>
int21_dispatcher::dispatch(const int21_request& request, const std::vector<std::byte>& memory, dos_process_state& state) {
    if (request.ah == 0x02) {
        state.console_output.push_back(static_cast<char>(request.dl));
        return {};
    }
    if (request.ah == 0x09) {
        if (request.string_offset >= memory.size()) return std::unexpected(dos_error{"INT 21h/09 string offset is outside memory"});
        for (std::size_t offset = request.string_offset; offset < memory.size(); ++offset) {
            const auto character = static_cast<char>(std::to_integer<unsigned char>(memory[offset]));
            if (character == '$') return {};
            state.console_output.push_back(character);
        }
        return std::unexpected(dos_error{"INT 21h/09 string is not '$'-terminated"});
    }
    if (request.ah == 0x4c) {
        state.exit_code = request.al;
        return {};
    }
    return std::unexpected(dos_error{"unsupported INT 21h function"});
}

std::expected<int21_memory_result, dos_error>
int21_memory_dispatcher::dispatch(const int21_request& request, conventional_memory& memory) {
    if (request.ah == 0x48) {
        const auto segment = memory.allocate(request.bx);
        if (!segment) return std::unexpected(dos_error{segment.error().message});
        return int21_memory_result{.allocated_segment = *segment};
    }
    if (request.ah == 0x49) {
        const auto released = memory.release(request.es);
        if (!released) return std::unexpected(dos_error{released.error().message});
        return int21_memory_result{};
    }
    return std::unexpected(dos_error{"unsupported INT 21h memory function"});
}

} // namespace dosrecomp::runtime
