/** @file straight_line_compiler.hpp @brief End-to-end recompilation of straight-line DOS programs via the full pipeline. */
#pragma once

#include "dosrecomp/loader/binary_loader.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::compiler {

/** Contextual reason a program cannot be compiled by the pipeline driver. */
struct straight_line_compile_error { std::string message; };

/**
 * Compiles DOS programs with constant-resolvable conditional branches by walking
 * the full CFG → IR → SSA pipeline and emitting a native ELF executable.
 *
 * At each conditional terminator the compiler statically evaluates the flag SSA
 * chain against the branch condition. Programs that require runtime branch
 * resolution are rejected with context. The supported termination is INT 21h
 * with AX holding the exit status.
 */
class straight_line_compiler final {
public:
    /** Extracts the preserved DOS process status from a constant-branch program. */
    [[nodiscard]] static std::expected<std::uint8_t, straight_line_compile_error>
    extract_exit_code(const loader::program_image& image);

    [[nodiscard]] static std::expected<std::vector<std::byte>, straight_line_compile_error>
    compile(const loader::program_image& image);

    /** Emits textual LLVM IR for the constant-branch DOS exit subset. */
    [[nodiscard]] static std::expected<std::string, straight_line_compile_error>
    emit_llvm(const loader::program_image& image);
};

} // namespace dosrecomp::compiler