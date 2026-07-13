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

[[nodiscard]] bool is_prefix(std::uint8_t opcode) {
    return opcode == 0x26 || opcode == 0x2e || opcode == 0x36 || opcode == 0x3e ||
           opcode == 0xf0 || opcode == 0xf2 || opcode == 0xf3;
}

[[nodiscard]] register_name register_for(std::uint8_t encoding, operand_width width) {
    constexpr std::array byte_registers{register_name::al, register_name::cl, register_name::dl, register_name::bl,
                                        register_name::ah, register_name::ch, register_name::dh, register_name::bh};
    constexpr std::array word_registers{register_name::ax, register_name::cx, register_name::dx, register_name::bx,
                                        register_name::sp, register_name::bp, register_name::si, register_name::di};
    return width == operand_width::byte ? byte_registers[encoding] : word_registers[encoding];
}

[[nodiscard]] instruction immediate_move(std::size_t offset, operand_width width, std::uint8_t encoding,
                                         std::uint16_t immediate) {
    const auto size = static_cast<std::uint8_t>(width == operand_width::byte ? 2 : 3);
    return instruction{instruction_kind::move_immediate, offset, size, 0, 0,
        {operand{operand_kind::reg, width, register_for(encoding, width), 0, 0},
         operand{operand_kind::immediate, width, register_name::al, immediate, 0}}, 2};
}
[[nodiscard]] operand register_operand(operand_width width, std::uint8_t encoding) {
    return operand{operand_kind::reg, width, register_for(encoding, width), 0, 0};
}
[[nodiscard]] operand rm_operand(const std::vector<std::byte>& code, std::size_t offset, operand_width width, std::uint8_t modrm) {
    if ((modrm >> 6U) == 3) return register_operand(width, modrm & 7U);
    constexpr std::array<std::array<register_name, 2>, 8> addresses{{
        {register_name::bx, register_name::si}, {register_name::bx, register_name::di},
        {register_name::bp, register_name::si}, {register_name::bp, register_name::di},
        {register_name::si, register_name::al}, {register_name::di, register_name::al},
        {register_name::bp, register_name::al}, {register_name::bx, register_name::al}}};
    const auto mode = modrm >> 6U;
    const auto rm = modrm & 7U;
    std::int16_t displacement = 0;
    std::uint8_t count = rm <= 3 ? 2 : 1;
    if (mode == 0 && rm == 6) {
        displacement = static_cast<std::int16_t>(static_cast<std::uint16_t>(byte_at(code, offset + 1)) |
                                                 (static_cast<std::uint16_t>(byte_at(code, offset + 2)) << 8U));
        count = 0;
    } else if (mode == 1) {
        displacement = static_cast<std::int8_t>(byte_at(code, offset + 1));
    } else if (mode == 2) {
        displacement = static_cast<std::int16_t>(static_cast<std::uint16_t>(byte_at(code, offset + 1)) |
                                                 (static_cast<std::uint16_t>(byte_at(code, offset + 2)) << 8U));
    }
    return operand{operand_kind::memory, width, register_name::al, 0, modrm, addresses[rm], count, displacement};
}
[[nodiscard]] instruction accumulator_immediate(instruction_kind kind, const std::vector<std::byte>& code,
                                                 std::size_t offset, std::uint8_t opcode) {
    const auto width = (opcode & 1U) == 0 ? operand_width::byte : operand_width::word;
    const auto size = static_cast<std::uint8_t>(width == operand_width::byte ? 2 : 3);
    std::uint16_t immediate = static_cast<std::uint16_t>(byte_at(code, offset + 1));
    if (width == operand_width::word) {
        immediate = static_cast<std::uint16_t>(immediate | (static_cast<std::uint16_t>(byte_at(code, offset + 2)) << 8U));
    }
    const auto accumulator = width == operand_width::byte ? register_name::al : register_name::ax;
    return instruction{kind, offset, size, 0, 0,
        {operand{operand_kind::reg, width, accumulator, 0, 0},
         operand{operand_kind::immediate, width, register_name::al, immediate, 0}}, 2};
}
} // namespace

std::expected<instruction, decode_error>
instruction_decoder::decode_at(const std::vector<std::byte>& code, std::size_t offset) {
    if (offset >= code.size()) {
        return std::unexpected(decode_error{"instruction offset is outside the code image"});
    }
    const auto opcode = byte_at(code, offset);
    if (is_prefix(opcode)) {
        std::size_t next = offset;
        std::uint8_t prefix_count = 0;
        while (next < code.size() && is_prefix(byte_at(code, next))) {
            if (prefix_count == 15) return std::unexpected(decode_error{"8086 instruction has too many prefixes"});
            ++prefix_count;
            ++next;
        }
        if (next == code.size()) return truncated(offset);
        const auto decoded = decode_at(code, next);
        if (!decoded) return std::unexpected(decoded.error());
        auto prefixed = *decoded;
        if (prefixed.size > static_cast<std::uint8_t>(15U - prefix_count)) {
            return std::unexpected(decode_error{"8086 instruction exceeds the 15-byte limit"});
        }
        prefixed.offset = offset;
        prefixed.size = static_cast<std::uint8_t>(prefixed.size + prefix_count);
        return prefixed;
    }
    const auto relative8 = [&]() -> std::expected<instruction, decode_error> {
        if (code.size() - offset < 2) return truncated(offset);
        return instruction{instruction_kind::jump, offset, 2,
            static_cast<std::int8_t>(byte_at(code, offset + 1)), 0};
    };
    if (opcode == 0x90) return instruction{instruction_kind::nop, offset, 1, 0, 0};
    if (opcode >= 0x91 && opcode <= 0x97) return instruction{instruction_kind::move, offset, 1, 0, 0};
    if (opcode == 0xd7) return instruction{instruction_kind::move, offset, 1, 0, 0};
    if (opcode == 0x98 || opcode == 0x99) return instruction{instruction_kind::arithmetic, offset, 1, 0, 0};
    if (opcode == 0x27 || opcode == 0x2f || opcode == 0x37 || opcode == 0x3f) {
        return instruction{instruction_kind::arithmetic, offset, 1, 0, 0};
    }
    if (opcode == 0xd4 || opcode == 0xd5) {
        if (code.size() - offset < 2) return truncated(offset);
        return instruction{instruction_kind::arithmetic, offset, 2, 0, 0};
    }
    if (opcode >= 0xa0 && opcode <= 0xa3) {
        if (code.size() - offset < 3) return truncated(offset);
        return instruction{instruction_kind::move, offset, 3, 0, 0};
    }
    if (opcode == 0xf4 || opcode == 0xf5 || (opcode >= 0xf8 && opcode <= 0xfd)) {
        return instruction{instruction_kind::flags, offset, 1, 0, 0};
    }
    if (opcode == 0x9e || opcode == 0x9f) return instruction{instruction_kind::flags, offset, 1, 0, 0};
    if (opcode == 0x9c || opcode == 0x9d || opcode == 0x0f) return instruction{instruction_kind::stack, offset, 1, 0, 0};
    if (opcode == 0x9b) return instruction{instruction_kind::nop, offset, 1, 0, 0};
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
        return immediate_move(offset, operand_width::byte, opcode - 0xb0U, byte_at(code, offset + 1));
    }
    if (opcode >= 0xb8 && opcode <= 0xbf) {
        if (code.size() - offset < 3) return truncated(offset);
        const auto immediate = static_cast<std::uint16_t>(byte_at(code, offset + 1)) |
                               (static_cast<std::uint16_t>(byte_at(code, offset + 2)) << 8U);
        return immediate_move(offset, operand_width::word, opcode - 0xb8U, immediate);
    }
    if (opcode == 0x04 || opcode == 0x05 || opcode == 0x0c || opcode == 0x0d || opcode == 0x14 || opcode == 0x15 ||
        opcode == 0x1c || opcode == 0x1d || opcode == 0x24 || opcode == 0x25 || opcode == 0x2c || opcode == 0x2d ||
        opcode == 0x34 || opcode == 0x35 || opcode == 0x3c || opcode == 0x3d || opcode == 0xa8 || opcode == 0xa9) {
        const auto size = static_cast<std::uint8_t>((opcode & 0x01U) == 0 ? 2U : 3U);
        if (code.size() - offset < size) return truncated(offset);
        const auto kind = (opcode & 0xf8U) == 0x38U || opcode == 0xa8 || opcode == 0xa9
            ? instruction_kind::compare : instruction_kind::arithmetic;
        return accumulator_immediate(kind, code, offset, opcode);
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
    if (opcode >= 0x88 && opcode <= 0x8b) {
        const auto size = modrm_size(code, offset + 1, 0);
        if (!size) return std::unexpected(size.error());
        const auto width = (opcode & 1U) == 0 ? operand_width::byte : operand_width::word;
        const auto modrm = byte_at(code, offset + 1);
        const auto reg = register_operand(width, (modrm >> 3U) & 7U);
        const auto rm = rm_operand(code, offset + 1, width, modrm);
        const auto destination = (opcode & 2U) == 0 ? rm : reg;
        const auto source = (opcode & 2U) == 0 ? reg : rm;
        return instruction{instruction_kind::move, offset, *size, 0, 0, {destination, source}, 2};
    }
    if ((opcode >= 0x8c && opcode <= 0x8e) || opcode == 0x8d || opcode == 0x8f || opcode == 0xc4 || opcode == 0xc5) {
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
    if (opcode == 0xf6 || opcode == 0xf7) {
        if (offset + 1 >= code.size()) return truncated(offset);
        const auto extension = (byte_at(code, offset + 1) >> 3U) & 0x07U;
        const auto immediate_size = extension == 0 ? static_cast<std::uint8_t>(opcode == 0xf6 ? 1 : 2) : 0;
        const auto size = modrm_size(code, offset + 1, immediate_size);
        if (!size) return std::unexpected(size.error());
        return instruction{extension == 0 ? instruction_kind::compare : instruction_kind::arithmetic, offset, *size, 0, 0};
    }
    if (opcode == 0xfe) {
        const auto size = modrm_size(code, offset + 1, 0);
        if (!size) return std::unexpected(size.error());
        const auto extension = (byte_at(code, offset + 1) >> 3U) & 0x07U;
        if (extension > 1) return std::unexpected(decode_error{"unsupported FE group extension"});
        return instruction{instruction_kind::arithmetic, offset, *size, 0, 0};
    }
    if (opcode >= 0xd0 && opcode <= 0xd3) {
        const auto size = modrm_size(code, offset + 1, 0);
        if (!size) return std::unexpected(size.error());
        return instruction{instruction_kind::arithmetic, offset, *size, 0, 0};
    }
    if (opcode >= 0xd8 && opcode <= 0xdf) {
        const auto size = modrm_size(code, offset + 1, 0);
        if (!size) return std::unexpected(size.error());
        return instruction{instruction_kind::coprocessor, offset, *size, 0, 0};
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
