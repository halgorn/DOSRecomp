/** @file elf_writer.hpp @brief Minimal validated ELF64/x86_64 image emission primitives. */
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace dosrecomp::backend {

/** A single write(1, buf, len) syscall embedded in a generated executable. */
struct write_call {
    std::span<const std::byte> payload;
    std::uint32_t file_descriptor{1};
};

/** Emits a standalone ELF64 executable that terminates through Linux syscall exit. */
class elf_writer final {
public:
    [[nodiscard]] static std::vector<std::byte> emit_exit_executable(std::uint8_t exit_code);

    /** Emits an ELF64 executable that writes payload to stdout and then exits. */
    [[nodiscard]] static std::vector<std::byte>
    emit_write_exit_executable(std::span<const std::byte> payload, std::uint8_t exit_code);

    /** Emits an ELF64 executable that performs multiple write syscalls and then exits. */
    [[nodiscard]] static std::vector<std::byte>
    emit_multi_write_exit_executable(std::span<const write_call> writes, std::uint8_t exit_code);
};

} // namespace dosrecomp::backend
