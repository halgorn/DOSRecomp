/** @file elf_writer.hpp @brief Minimal validated ELF64/x86_64 image emission primitives. */
#pragma once

#include <cstdint>
#include <vector>

namespace dosrecomp::backend {

/** Emits a standalone ELF64 executable that terminates through Linux syscall exit. */
class elf_writer final {
public:
    [[nodiscard]] static std::vector<std::byte> emit_exit_executable(std::uint8_t exit_code);
};

} // namespace dosrecomp::backend

