/** @file conventional_memory.hpp @brief Paragraph allocator for DOS conventional memory. */
#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::runtime {

struct memory_error { std::string message; };

/**
 * Allocates DOS memory in 16-byte paragraphs from a bounded conventional pool.
 * Segment values are pool-relative and suitable for real-mode address modeling.
 */
class conventional_memory final {
public:
    explicit conventional_memory(std::uint16_t paragraphs = 40'960);

    [[nodiscard]] std::expected<std::uint16_t, memory_error> allocate(std::uint16_t paragraphs);
    [[nodiscard]] std::expected<void, memory_error> release(std::uint16_t segment);
    [[nodiscard]] std::uint16_t largest_free_block() const noexcept;

private:
    struct block { std::uint16_t start{}; std::uint16_t size{}; bool free{}; };
    std::vector<block> blocks_;
};

} // namespace dosrecomp::runtime

