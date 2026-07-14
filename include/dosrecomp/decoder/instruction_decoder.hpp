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

/** 8086 condition encoded by Jcc; `always` is used by non-conditional instructions. */
enum class branch_condition : std::uint8_t {
    overflow, not_overflow, below, above_or_equal, equal, not_equal, below_or_equal,
    above, sign, not_sign, parity, not_parity, less, greater_or_equal, less_or_equal,
    greater, loop_not_equal, loop_equal, loop, cx_zero, always
};

/** Arithmetic/logic operation encoded by a decoded instruction. */
enum class alu_operation : std::uint8_t { none, add, adc, bit_or, sbb, bit_and, subtract, bit_xor, compare, test };

/** A decoded operand; memory forms retain their ModR/M byte for later lowering. */
struct operand {
    operand_kind kind{operand_kind::none};
    operand_width width{operand_width::byte};
    register_name reg{register_name::al};
    std::uint16_t immediate{};
    std::uint8_t modrm{};
    std::array<register_name, 2> address_registers{};
    std::uint8_t address_register_count{};
    std::int16_t displacement{};
    bool direct_address{};
};

/** REP/REPZ/REPNZ prefix encoded on a string instruction. */
enum class rep_prefix : std::uint8_t { none = 0, rep = 1, repe = 2, repne = 3 };

/** One instruction boundary and its optional relative target. */
struct instruction {
    instruction_kind kind{};
    std::size_t offset{};
    std::uint8_t size{};
    std::int32_t relative_target{};
    std::uint8_t interrupt_number{};
    std::array<operand, 2> operands{};
    std::uint8_t operand_count{};
    branch_condition condition{branch_condition::always};
    alu_operation alu{alu_operation::none};
    bool stack_pop{false};
    bool indirect{false};
    rep_prefix rep{rep_prefix::none};
    bool operand_size_override{false};
    bool address_size_override{false};
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
