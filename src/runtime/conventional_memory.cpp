#include "dosrecomp/runtime/conventional_memory.hpp"

#include <algorithm>

namespace dosrecomp::runtime {
conventional_memory::conventional_memory(std::uint16_t paragraphs) {
    if (paragraphs != 0) blocks_.push_back({.start = 0, .size = paragraphs, .free = true});
}

std::expected<std::uint16_t, memory_error> conventional_memory::allocate(std::uint16_t paragraphs) {
    if (paragraphs == 0) return std::unexpected(memory_error{"DOS allocation size must be non-zero"});
    for (std::size_t index = 0; index < blocks_.size(); ++index) {
        auto& block = blocks_[index];
        if (!block.free || block.size < paragraphs) continue;
        const auto segment = block.start;
        if (block.size == paragraphs) {
            block.free = false;
        } else {
            const auto remainder_start = static_cast<std::uint16_t>(block.start + paragraphs);
            const auto remainder_size = static_cast<std::uint16_t>(block.size - paragraphs);
            block.size = paragraphs;
            block.free = false;
            blocks_.insert(blocks_.begin() + static_cast<std::ptrdiff_t>(index + 1),
                {.start = remainder_start, .size = remainder_size, .free = true});
        }
        return segment;
    }
    return std::unexpected(memory_error{"DOS conventional memory is exhausted"});
}

std::expected<void, memory_error> conventional_memory::release(std::uint16_t segment) {
    const auto found = std::find_if(blocks_.begin(), blocks_.end(), [segment](const block& item) { return item.start == segment; });
    if (found == blocks_.end() || found->free) return std::unexpected(memory_error{"DOS memory segment was not allocated"});
    found->free = true;
    for (std::size_t index = 0; index + 1 < blocks_.size();) {
        auto& current = blocks_[index];
        const auto& next = blocks_[index + 1];
        if (current.free && next.free) {
            current.size = static_cast<std::uint16_t>(current.size + next.size);
            blocks_.erase(blocks_.begin() + static_cast<std::ptrdiff_t>(index + 1));
        } else {
            ++index;
        }
    }
    return {};
}

std::uint16_t conventional_memory::largest_free_block() const noexcept {
    std::uint16_t largest = 0;
    for (const auto& block : blocks_) if (block.free && block.size > largest) largest = block.size;
    return largest;
}

} // namespace dosrecomp::runtime
