#include "dosrecomp/runtime/mode13.hpp"

namespace dosrecomp::runtime {
namespace {
std::expected<std::size_t, video_memory_error> offset_for(std::size_t x, std::size_t y) {
    if (x >= mode13_framebuffer::width || y >= mode13_framebuffer::height) {
        return std::unexpected(video_memory_error{"Mode 13h pixel coordinate is outside 320x200"});
    }
    return y * mode13_framebuffer::width + x;
}
}

std::expected<void, video_memory_error> mode13_framebuffer::set_pixel(std::size_t x, std::size_t y, std::uint8_t color) {
    const auto offset = offset_for(x, y);
    if (!offset) return std::unexpected(offset.error());
    pixels_[*offset] = color;
    return {};
}

std::expected<std::uint8_t, video_memory_error> mode13_framebuffer::pixel(std::size_t x, std::size_t y) const {
    const auto offset = offset_for(x, y);
    if (!offset) return std::unexpected(offset.error());
    return pixels_[*offset];
}

void mode13_framebuffer::set_palette(std::uint8_t index, rgb_color color) noexcept { palette_[index] = color; }
rgb_color mode13_framebuffer::palette(std::uint8_t index) const noexcept { return palette_[index]; }

} // namespace dosrecomp::runtime

