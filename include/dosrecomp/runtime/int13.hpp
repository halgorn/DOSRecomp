/** @file int13.hpp @brief Validated virtual-disk subset of BIOS INT 13h. */
#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::runtime {

struct disk_geometry { std::uint16_t cylinders{}; std::uint8_t heads{}; std::uint8_t sectors_per_track{}; };
struct virtual_disk { disk_geometry geometry; std::vector<std::byte> bytes; };
struct int13_request { std::uint8_t ah{}; std::uint8_t al{}; std::uint8_t ch{}; std::uint8_t cl{}; std::uint8_t dh{}; std::uint8_t dl{}; };
struct int13_result { std::vector<std::byte> data; std::uint8_t sectors_transferred{}; };
struct disk_error { std::string message; };

/** Implements BIOS read-sectors (`AH=02h`) against an in-memory virtual disk. */
class int13_dispatcher final {
public:
    [[nodiscard]] static std::expected<int13_result, disk_error>
    dispatch(const int13_request& request, const virtual_disk& disk);
};

} // namespace dosrecomp::runtime

