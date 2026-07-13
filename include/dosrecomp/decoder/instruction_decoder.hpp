/**
 * @file instruction_decoder.hpp
 * @brief Bounds-checked decoding of the control-flow subset of 8086 machine code.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::decoder {

/** The decoded operation categories currently needed for CFG recovery. */
enum class instruction_kind {
    nop, move_immediate, move, arithmetic, compare, stack, flags, string, io, coprocessor, interrupt, call, jump, conditional_jump, loop, return_
};

/** Width of one explicitly decoded 8086 operand. */
enum class operand_width : std::uint8_t { byte = 8, word = 16 };

/** Register names shared by byte, word, and segment operand decoding. */
enum class register_name : std::uint8_t {
    al, cl, dl, bl, ah, ch, dh, bh,
    ax, cx, dx, bx, sp, bp, si, di,
    es, cs, ss, ds
};

/** Operand category. Memory remains explicit until effective-address lowering. */
enum class operand_kind : std::uint8_t { none, reg, immediate, memory };

/** A decoded operand; memory forms retain their ModR/M byte for later lowering. */
struct operand {
    operand_kind kind{operand_kind::none};
    operand_width width{operand_width::byte};
    register_name reg{register_name::al};
    std::uint16_t immediate{};
    std::uint8_t modrm{};
};

/** One instruction boundary and its optional relative target. */
struct instruction {
    instruction_kind kind{};
    std::size_t offset{};
    std::uint8_t size{};
    std::int32_t relative_target{};
    std::uint8_t interrupt_number{};
    std::array<operand, 2> operands{};
    std::uint8_t operand_count{};
};

/** A malformed, truncated, or not-yet-supported instruction. */
struct decode_error { std::string message; };

/**
 * Decodes a safe, CFG-oriented 8086 subset without executing it.
 *
 * Unknown instructions are rejected rather than assigned an invented length.
 * It recognizes common ModR/M instruction boundaries and exposes structured
 * register/immediate operands where decoding is proven. Remaining operand
 * semantics and encodings are intentionally a later module.
 */
class instruction_decoder final {
public:
    [[nodiscard]] static std::expected<instruction, decode_error>
    decode_at(const std::vector<std::byte>& code, std::size_t offset);
};

} // namespace dosrecomp::decoder
