/** @file cpp_backend.hpp @brief Emits readable C++17 source from a straight-line DOS program. */
#pragma once

#include "dosrecomp/loader/binary_loader.hpp"

#include <expected>
#include <string>

namespace dosrecomp::compiler {

/** Contextual reason a program cannot be emitted as C++. */
struct cpp_emit_error { std::string message; };

/**
 * Translates the verified straight-line DOS subset into a readable C++ program.
 *
 * The emitted source maps 8086 registers to local variables, evaluates all
 * straight-line semantics at translation time, and replaces INT 21h handlers
 * with calls to the host dos_int21 helper.
 */
class cpp_backend final {
public:
    [[nodiscard]] static std::expected<std::string, cpp_emit_error>
    emit(const loader::program_image& image);
};

} // namespace dosrecomp::compiler