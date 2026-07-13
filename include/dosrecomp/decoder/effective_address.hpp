/** @file effective_address.hpp @brief Pure 8086 effective-offset resolution. */
#pragma once

#include "dosrecomp/decoder/instruction_decoder.hpp"

#include <array>
#include <cstdint>
#include <expected>
#include <string>

namespace dosrecomp::decoder {

struct effective_address_error { std::string message; };
using register_values = std::array<std::uint16_t, static_cast<std::size_t>(register_name::ds) + 1>;

/** Resolves a decoded memory operand to its wrapping 16-bit offset. */
class effective_address_resolver final {
public:
    [[nodiscard]] static std::expected<std::uint16_t, effective_address_error>
    resolve(const operand& memory, const register_values& registers);
};

} // namespace dosrecomp::decoder
