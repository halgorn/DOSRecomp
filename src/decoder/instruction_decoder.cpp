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

[[nodiscard]] std::expected<std::uint8_t, decode_error>
modrm_size(const std::vector<std::byte>& code, std::size_t offset, std::uint8_t immediate_size) {
    if (offset >= code.size()) return std::unexpected(decode_error{"truncated instruction at offset " + std::to_string(offset)});
    const auto modrm = byte_at(code, offset);
    const auto mode = modrm >> 6U;
    const auto rm = modrm & 0x07U;
    std::uint8_t displacement_size = 0;
    if (mode == 0 && rm == 6) displacement_size = 2;
    if (mode == 1) displacement_size = 1;
    if (mode == 2) displacement_size = 2;
    const auto size = static_cast<std::uint8_t>(2U + displacement_size + immediate_size);
    if (code.size() - offset < static_cast<std::size_t>(size - 1U)) {
        return std::unexpected(decode_error{"truncated instruction at offset " + std::to_string(offset - 1U)});
    }
    return size;
}

[[nodiscard]] bool is_modrm_arithmetic(std::uint8_t opcode) {
    return (opcode <= 0x3b && (opcode & 0x04U) != 0x04U) || opcode == 0x84 || opcode == 0x85 ||
           opcode == 0x86 || opcode == 0x87;
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
    if (opcode >= 0x91 && opcode <= 0x97) return instruction{instruction_kind::move, offset, 1, 0, 0};
    if (opcode == 0xf4 || opcode == 0xf5 || (opcode >= 0xf8 && opcode <= 0xfd)) {
        return instruction{instruction_kind::flags, offset, 1, 0, 0};
    }
    if ((opcode >= 0xa4 && opcode <= 0xa7) || (opcode >= 0xaa && opcode <= 0xaf)) {
        return instruction{instruction_kind::string, offset, 1, 0, 0};
    }
    if (opcode == 0xe4 || opcode == 0xe5 || opcode == 0xe6 || opcode == 0xe7) {
        if (code.size() - offset < 2) return truncated(offset);
        return instruction{instruction_kind::io, offset, 2, 0, 0};
    }
    if (opcode >= 0xec && opcode <= 0xef) return instruction{instruction_kind::io, offset, 1, 0, 0};
    if (opcode >= 0xb0 && opcode <= 0xb7) {
        if (code.size() - offset < 2) return truncated(offset);
        return instruction{instruction_kind::move_immediate, offset, 2, 0, 0};
    }
    if (opcode >= 0xb8 && opcode <= 0xbf) {
        if (code.size() - offset < 3) return truncated(offset);
        return instruction{instruction_kind::move_immediate, offset, 3, 0, 0};
    }
    if (opcode == 0x04 || opcode == 0x05 || opcode == 0x0c || opcode == 0x0d || opcode == 0x14 || opcode == 0x15 ||
        opcode == 0x1c || opcode == 0x1d || opcode == 0x24 || opcode == 0x25 || opcode == 0x2c || opcode == 0x2d ||
        opcode == 0x34 || opcode == 0x35 || opcode == 0x3c || opcode == 0x3d || opcode == 0xa8 || opcode == 0xa9) {
        const auto size = static_cast<std::uint8_t>((opcode & 0x01U) == 0 ? 2U : 3U);
        if (code.size() - offset < size) return truncated(offset);
        const auto kind = (opcode & 0xf8U) == 0x38U || opcode == 0xa8 || opcode == 0xa9
            ? instruction_kind::compare : instruction_kind::arithmetic;
        return instruction{kind, offset, size, 0, 0};
    }
    if ((opcode >= 0x50 && opcode <= 0x5f) || opcode == 0x06 || opcode == 0x07 || opcode == 0x0e || opcode == 0x16 || opcode == 0x17 || opcode == 0x1e || opcode == 0x1f) {
        return instruction{instruction_kind::stack, offset, 1, 0, 0};
    }
    if (is_modrm_arithmetic(opcode)) {
        const auto size = modrm_size(code, offset + 1, 0);
        if (!size) return std::unexpected(size.error());
        const auto kind = (opcode & 0xf8U) == 0x38U || opcode == 0x84 || opcode == 0x85
            ? instruction_kind::compare : instruction_kind::arithmetic;
        return instruction{kind, offset, *size, 0, 0};
    }
    if ((opcode >= 0x88 && opcode <= 0x8e) || opcode == 0x8d || opcode == 0x8f) {
        const auto size = modrm_size(code, offset + 1, 0);
        if (!size) return std::unexpected(size.error());
        return instruction{instruction_kind::move, offset, *size, 0, 0};
    }
    if (opcode == 0xc6 || opcode == 0xc7 || opcode == 0x80 || opcode == 0x82 || opcode == 0x83 || opcode == 0x81) {
        const auto immediate_size = (opcode == 0xc7 || opcode == 0x81) ? 2U : 1U;
        const auto size = modrm_size(code, offset + 1, immediate_size);
        if (!size) return std::unexpected(size.error());
        return instruction{opcode == 0xc6 || opcode == 0xc7 ? instruction_kind::move : instruction_kind::arithmetic,
            offset, *size, 0, 0};
    }
    if (opcode == 0xff) {
        const auto size = modrm_size(code, offset + 1, 0);
        if (!size) return std::unexpected(size.error());
        const auto extension = (byte_at(code, offset + 1) >> 3U) & 0x07U;
        if (extension >= 2 && extension <= 5) {
            return std::unexpected(decode_error{"indirect 8086 control flow is not supported"});
        }
        return instruction{extension == 6 ? instruction_kind::stack : instruction_kind::arithmetic, offset, *size, 0, 0};
    }
    if (opcode == 0xcd) {
        if (code.size() - offset < 2) return truncated(offset);
        return instruction{instruction_kind::interrupt, offset, 2, 0, byte_at(code, offset + 1)};
    }
    if (opcode == 0xcc) return instruction{instruction_kind::interrupt, offset, 1, 0, 3};
    if (opcode == 0xce) return instruction{instruction_kind::interrupt, offset, 1, 0, 4};
    if (opcode == 0xc3 || opcode == 0xcb) return instruction{instruction_kind::return_, offset, 1, 0, 0};
    if (opcode == 0xcf) return instruction{instruction_kind::return_, offset, 1, 0, 0};
    if (opcode == 0xc2) {
        if (code.size() - offset < 3) return truncated(offset);
        return instruction{instruction_kind::return_, offset, 3, 0, 0};
    }
    if (opcode == 0xca) {
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
