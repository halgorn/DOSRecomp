/** @file int10.hpp @brief Text-mode subset of the DOS BIOS video interrupt. */
#pragma once

#include <cstdint>
#include <expected>
#include <string>

namespace dosrecomp::runtime {

/** Registers consumed by implemented BIOS INT 10h text functions. */
struct int10_request { std::uint8_t ah{}; std::uint8_t al{}; std::uint8_t dh{}; std::uint8_t dl{}; };

/** Host-independent text terminal state. */
struct text_video_state { std::string output; std::uint8_t row{}; std::uint8_t column{}; };
struct video_error { std::string message; };

/** Implements teletype output and cursor positioning for text-mode programs. */
class int10_dispatcher final {
public:
    [[nodiscard]] static std::expected<void, video_error>
    dispatch(const int10_request& request, text_video_state& state);
};

} // namespace dosrecomp::runtime

