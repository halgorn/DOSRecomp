/**
 * @file instruction_decoder.hpp
 * @brief Bounds-checked decoding of the control-flow subset of 8086 machine code.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::decoder {

/** The decoded operation categories currently needed for CFG recovery. */
enum class instruction_kind {
    nop, move_immediate, move, arithmetic, compare, stack, flags, interrupt, call, jump, conditional_jump, loop, return_
};

/** One instruction boundary and its optional relative target. */
struct instruction {
    instruction_kind kind{};
    std::size_t offset{};
    std::uint8_t size{};
    std::int32_t relative_target{};
    std::uint8_t interrupt_number{};
};

/** A malformed, truncated, or not-yet-supported instruction. */
struct decode_error { std::string message; };

/**
 * Decodes a safe, CFG-oriented 8086 subset without executing it.
 *
 * Unknown instructions are rejected rather than assigned an invented length.
 * It recognizes common ModR/M instruction boundaries; operand semantics and
 * the remaining 8086 encodings are intentionally a later module.
 */
class instruction_decoder final {
public:
    [[nodiscard]] static std::expected<instruction, decode_error>
    decode_at(const std::vector<std::byte>& code, std::size_t offset);
};

} // namespace dosrecomp::decoder
