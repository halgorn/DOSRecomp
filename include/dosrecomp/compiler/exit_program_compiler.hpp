/** @file exit_program_compiler.hpp @brief End-to-end recompilation of the verified DOS exit subset. */
#pragma once

#include "dosrecomp/loader/binary_loader.hpp"

#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::compiler {

/** Contextual reason a program cannot be compiled by the current semantic subset. */
struct compile_error { std::string message; };

/**
 * Compiles a verified DOS exit sequence, plus the COM `INT 21h/AH=09h` console
 * output sequence followed by an exit, into a native ELF executable.
 *
 * This deliberately narrow vertical slice proves loader → decoder → semantics
 * → backend composition. Other program shapes are rejected until translated.
 */
class exit_program_compiler final {
public:
    /** Extracts the preserved DOS process status from the supported entry sequence. */
    [[nodiscard]] static std::expected<std::uint8_t, compile_error>
    extract_exit_code(const loader::program_image& image);

    [[nodiscard]] static std::expected<std::vector<std::byte>, compile_error>
    compile(const loader::program_image& image);

    /** Emits textual LLVM IR for the same verified DOS exit subset. */
    [[nodiscard]] static std::expected<std::string, compile_error>
    emit_llvm(const loader::program_image& image);
};

} // namespace dosrecomp::compiler
