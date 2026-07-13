/**
 * @file binary_loader.hpp
 * @brief Validated loaders for real-mode DOS COM and MZ executables.
 *
 * The loader deliberately does not execute code. It produces the load-module
 * representation consumed by the decoder and control-flow analysis layers.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace dosrecomp::loader {

/** The executable container recognized by the loader. */
enum class executable_format { com, mz };

/** A DOS segment:offset address, relative to the program load module. */
struct segmented_address {
    std::uint16_t segment{};
    std::uint16_t offset{};
};

/** A relocation word address inside an MZ load module. */
struct relocation {
    segmented_address address{};
};

/** Immutable input to later recompilation stages. */
struct program_image {
    executable_format format{};
    std::vector<std::byte> bytes;
    segmented_address entry_point{};
    segmented_address initial_stack{};
    std::vector<relocation> relocations;

    /** Returns the entry index relative to @ref bytes, not the DOS logical address. */
    [[nodiscard]] constexpr std::size_t entry_offset() const noexcept {
        return format == executable_format::com
            ? 0U
            : static_cast<std::size_t>(entry_point.segment) * 16U + entry_point.offset;
    }
};

/** A contextual explanation for an unreadable or malformed input binary. */
struct load_error {
    std::string message;
};

/**
 * Loads DOS program containers while validating all on-disk bounds.
 *
 * COM files are exposed at their conventional offset 0100h. MZ relocation
 * entries are retained and can be applied to an explicit load segment.
 */
class binary_loader final {
public:
    [[nodiscard]] static std::expected<program_image, load_error>
    load_file(const std::filesystem::path& path);

    [[nodiscard]] static std::expected<program_image, load_error>
    load_bytes(const std::vector<std::byte>& file);

    /** Returns a relocated MZ image without mutating the original input image. */
    [[nodiscard]] static std::expected<program_image, load_error>
    apply_relocations(const program_image& image, std::uint16_t load_segment);
};

} // namespace dosrecomp::loader
