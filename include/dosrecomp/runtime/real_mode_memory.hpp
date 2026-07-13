/** @file real_mode_memory.hpp @brief Bounds-checked 8086 physical memory. */
#pragma once

#include "dosrecomp/runtime/segmented_address.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::runtime {

struct memory_error { std::string message; };

/** Owns the complete addressable 8086 real-mode physical memory range. */
class real_mode_memory final {
public:
    static constexpr std::size_t size = 0x110000;
    real_mode_memory();
    [[nodiscard]] std::expected<std::uint8_t, memory_error> read8(std::uint32_t address) const;
    [[nodiscard]] std::expected<void, memory_error> write8(std::uint32_t address, std::uint8_t value);
    [[nodiscard]] std::expected<std::uint16_t, memory_error> read16(std::uint32_t address) const;
    [[nodiscard]] std::expected<void, memory_error> write16(std::uint32_t address, std::uint16_t value);
    [[nodiscard]] std::expected<std::uint8_t, memory_error> read8(std::uint16_t segment, std::uint16_t offset) const;
    [[nodiscard]] std::expected<void, memory_error> write8(std::uint16_t segment, std::uint16_t offset, std::uint8_t value);
    [[nodiscard]] std::expected<std::uint16_t, memory_error> read16(std::uint16_t segment, std::uint16_t offset) const;
    [[nodiscard]] std::expected<void, memory_error> write16(std::uint16_t segment, std::uint16_t offset, std::uint16_t value);
private:
    std::vector<std::byte> bytes_;
};
} // namespace dosrecomp::runtime
