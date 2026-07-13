/** @file elf_writer.hpp @brief Minimal validated ELF64/x86_64 image emission primitives. */
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace dosrecomp::backend {

/** Emits a standalone ELF64 executable that terminates through Linux syscall exit. */
class elf_writer final {
public:
    [[nodiscard]] static std::vector<std::byte> emit_exit_executable(std::uint8_t exit_code);

    /** Emits an ELF64 executable that writes payload to stdout and then exits. */
    [[nodiscard]] static std::vector<std::byte>
    emit_write_exit_executable(std::span<const std::byte> payload, std::uint8_t exit_code);
};

} // namespace dosrecomp::backend
