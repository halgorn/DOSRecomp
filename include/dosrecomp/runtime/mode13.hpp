/** @file mode13.hpp @brief Memory-safe 320x200x8 Mode 13h framebuffer model. */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>

namespace dosrecomp::runtime {

struct rgb_color { std::uint8_t red{}; std::uint8_t green{}; std::uint8_t blue{}; };
struct video_memory_error { std::string message; };

/** Stores Mode 13h indexed pixels and palette entries without a presentation backend. */
class mode13_framebuffer final {
public:
    static constexpr std::size_t width = 320;
    static constexpr std::size_t height = 200;

    [[nodiscard]] std::expected<void, video_memory_error> set_pixel(std::size_t x, std::size_t y, std::uint8_t color);
    [[nodiscard]] std::expected<std::uint8_t, video_memory_error> pixel(std::size_t x, std::size_t y) const;
    void set_palette(std::uint8_t index, rgb_color color) noexcept;
    [[nodiscard]] rgb_color palette(std::uint8_t index) const noexcept;

private:
    std::array<std::uint8_t, width * height> pixels_{};
    std::array<rgb_color, 256> palette_{};
};

} // namespace dosrecomp::runtime

