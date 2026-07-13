/** @file segmented_address.hpp @brief 8086 real-mode segment:offset translation. */
#pragma once

#include <cstdint>

namespace dosrecomp::runtime {

/** Translates a real-mode logical address into its 20-bit physical address. */
[[nodiscard]] constexpr std::uint32_t physical_address(std::uint16_t segment, std::uint16_t offset) noexcept {
    return (static_cast<std::uint32_t>(segment) << 4U) + offset;
}

} // namespace dosrecomp::runtime
