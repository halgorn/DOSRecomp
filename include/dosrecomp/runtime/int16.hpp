/** @file int16.hpp @brief Deterministic keyboard subset of BIOS INT 16h. */
#pragma once

#include <cstdint>
#include <deque>
#include <expected>
#include <optional>
#include <string>

namespace dosrecomp::runtime {

struct keyboard_key { std::uint8_t ascii{}; std::uint8_t scan_code{}; };
struct int16_request { std::uint8_t ah{}; };
struct int16_result { std::optional<keyboard_key> key; bool available{}; };
struct keyboard_error { std::string message; };

/** Implements INT 16h read (`AH=00h`) and peek (`AH=01h`) over a caller-owned queue. */
class int16_dispatcher final {
public:
    [[nodiscard]] static std::expected<int16_result, keyboard_error>
    dispatch(const int16_request& request, std::deque<keyboard_key>& queue);
};

} // namespace dosrecomp::runtime

