/** @file branching_compiler.hpp @brief Recompiles DOS programs with runtime branches to native ELF. */
#pragma once

#include "dosrecomp/loader/binary_loader.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::compiler {

/** Contextual reason a program cannot be recompiled by the branching compiler. */
struct branching_compile_error { std::string message; };

/**
 * Compiles DOS programs that include runtime data-dependent conditional branches
 * by emitting a native x86_64 executable with proper control flow.
 *
 * The compiler walks the CFG and translates each basic block into x86_64
 * machine code; data-dependent branches become real cmp + jcc sequences.
 * INT 21h handlers become Linux syscall sequences.
 */
class branching_compiler final {
public:
    [[nodiscard]] static std::expected<std::uint8_t, branching_compile_error>
    extract_exit_code(const loader::program_image& image);

    [[nodiscard]] static std::expected<std::vector<std::byte>, branching_compile_error>
    compile(const loader::program_image& image);

    [[nodiscard]] static std::expected<std::string, branching_compile_error>
    emit_llvm(const loader::program_image& image);
};

} // namespace dosrecomp::compiler