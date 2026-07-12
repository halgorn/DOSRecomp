#include "dosrecomp/decoder/instruction_decoder.hpp"

#include <sstream>

namespace dosrecomp::decoder {
namespace {
[[nodiscard]] std::uint8_t byte_at(const std::vector<std::byte>& code, std::size_t offset) {
    return std::to_integer<std::uint8_t>(code[offset]);
}

[[nodiscard]] std::expected<instruction, decode_error> truncated(std::size_t offset) {
    return std::unexpected(decode_error{"truncated instruction at offset " + std::to_string(offset)});
}

[[nodiscard]] std::int16_t read_i16(const std::vector<std::byte>& code, std::size_t offset) {
    const auto value = static_cast<std::uint16_t>(byte_at(code, offset)) |
                       static_cast<std::uint16_t>(byte_at(code, offset + 1)) << 8U;
    return static_cast<std::int16_t>(value);
}
} // namespace

std::expected<instruction, decode_error>
instruction_decoder::decode_at(const std::vector<std::byte>& code, std::size_t offset) {
    if (offset >= code.size()) {
        return std::unexpected(decode_error{"instruction offset is outside the code image"});
    }
    const auto opcode = byte_at(code, offset);
    const auto relative8 = [&]() -> std::expected<instruction, decode_error> {
        if (code.size() - offset < 2) return truncated(offset);
        return instruction{instruction_kind::jump, offset, 2,
            static_cast<std::int8_t>(byte_at(code, offset + 1)), 0};
    };
    if (opcode == 0x90) return instruction{instruction_kind::nop, offset, 1, 0, 0};
    if (opcode >= 0xb0 && opcode <= 0xb7) {
        if (code.size() - offset < 2) return truncated(offset);
        return instruction{instruction_kind::move_immediate, offset, 2, 0, 0};
    }
    if (opcode >= 0xb8 && opcode <= 0xbf) {
        if (code.size() - offset < 3) return truncated(offset);
        return instruction{instruction_kind::move_immediate, offset, 3, 0, 0};
    }
    if (opcode == 0xcd) {
        if (code.size() - offset < 2) return truncated(offset);
        return instruction{instruction_kind::interrupt, offset, 2, 0, byte_at(code, offset + 1)};
    }
    if (opcode == 0xc3) return instruction{instruction_kind::return_, offset, 1, 0, 0};
    if (opcode == 0xc2) {
        if (code.size() - offset < 3) return truncated(offset);
        return instruction{instruction_kind::return_, offset, 3, 0, 0};
    }
    if (opcode == 0xe8 || opcode == 0xe9) {
        if (code.size() - offset < 3) return truncated(offset);
        return instruction{opcode == 0xe8 ? instruction_kind::call : instruction_kind::jump,
            offset, 3, read_i16(code, offset + 1), 0};
    }
    if (opcode == 0xeb) return relative8();
    if (opcode >= 0x70 && opcode <= 0x7f) {
        if (code.size() - offset < 2) return truncated(offset);
        return instruction{instruction_kind::conditional_jump, offset, 2,
            static_cast<std::int8_t>(byte_at(code, offset + 1)), 0};
    }
    if (opcode >= 0xe0 && opcode <= 0xe3) {
        if (code.size() - offset < 2) return truncated(offset);
        return instruction{instruction_kind::loop, offset, 2,
            static_cast<std::int8_t>(byte_at(code, offset + 1)), 0};
    }
    std::ostringstream message;
    message << "unsupported 8086 opcode 0x" << std::hex << static_cast<unsigned>(opcode)
            << " at offset " << std::dec << offset;
    return std::unexpected(decode_error{message.str()});
}

} // namespace dosrecomp::decoder

