/** @file elf_writer.hpp @brief Minimal validated ELF64/x86_64 image emission primitives. */
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace dosrecomp::backend {

/** A single write(fd, buf, len) syscall embedded in a generated executable. */
struct write_call {
    std::span<const std::byte> payload;
    std::uint32_t file_descriptor{1};
};

/** A single read(fd, buf, len) syscall embedded in a generated executable. */
struct read_call {
    std::uint32_t file_descriptor{0};
    std::uint32_t max_bytes{1};
};

/** A single open(path, flags, mode) syscall embedded in a generated executable. */
struct open_call {
    std::span<const std::byte> filename;
    std::uint32_t flags{0x41};
    std::uint32_t mode{0x1ff};
};

/** A single close(fd) syscall embedded in a generated executable. */
struct close_call {
    std::uint32_t file_descriptor{0};
};

/** A descriptor for one syscall invocation in a generated executable. */
struct syscall_action {
    enum class kind { write, read, open, close } kind;
    union {
        write_call write{};
        read_call read;
        open_call open;
        close_call close;
    };
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

    /**
     * Emits an ELF64 executable driven by a sequence of write and read syscalls.
     * Each read syscall stores the returned byte count in rax (read-only); the
     * compiled payload is responsible for extracting AL where DOS semantics need it.
     */
    [[nodiscard]] static std::vector<std::byte>
    emit_syscall_program_executable(std::span<const write_call> writes,
                                    std::span<const read_call> reads,
                                    std::uint8_t exit_code);

    /**
     * Emits an ELF64 executable driven by an arbitrary sequence of file
     * syscalls followed by an exit. The fd returned by an open action is
     * threaded into the next write/close action via a register move.
     */
    [[nodiscard]] static std::vector<std::byte>
    emit_action_program_executable(std::span<const syscall_action> actions,
                                    std::uint8_t exit_code);
};

} // namespace dosrecomp::backend
