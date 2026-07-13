#include "dosrecomp/backend/elf_writer.hpp"

#include <algorithm>
#include <array>

namespace dosrecomp::backend {
namespace {
void write16(std::vector<std::byte>& bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = static_cast<std::byte>(value & 0xffU);
    bytes[offset + 1] = static_cast<std::byte>(value >> 8U);
}
void write32(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    for (std::size_t index = 0; index < 4; ++index) bytes[offset + index] = static_cast<std::byte>(value >> (index * 8U));
}
void write64(std::vector<std::byte>& bytes, std::size_t offset, std::uint64_t value) {
    for (std::size_t index = 0; index < 8; ++index) bytes[offset + index] = static_cast<std::byte>(value >> (index * 8U));
}
}

std::vector<std::byte> elf_writer::emit_exit_executable(std::uint8_t exit_code) {
    constexpr std::size_t header_size = 64;
    constexpr std::size_t program_header_size = 56;
    constexpr std::size_t code_offset = header_size + program_header_size;
    constexpr std::size_t code_size = 12;
    constexpr std::uint64_t image_base = 0x400000;
    std::vector<std::byte> image(code_offset + code_size);
    image[0] = std::byte{0x7f}; image[1] = std::byte{'E'}; image[2] = std::byte{'L'}; image[3] = std::byte{'F'};
    image[4] = std::byte{2}; image[5] = std::byte{1}; image[6] = std::byte{1};
    write16(image, 16, 2); write16(image, 18, 62); write32(image, 20, 1);
    write64(image, 24, image_base + code_offset); write64(image, 32, header_size);
    write16(image, 52, header_size); write16(image, 54, program_header_size); write16(image, 56, 1);
    write32(image, 64, 1); write32(image, 68, 5); write64(image, 72, 0);
    write64(image, 80, image_base); write64(image, 88, image_base);
    write64(image, 96, image.size()); write64(image, 104, image.size()); write64(image, 112, 0x1000);
    image[code_offset] = std::byte{0xb8}; image[code_offset + 1] = std::byte{0x3c};
    image[code_offset + 5] = std::byte{0xbf}; image[code_offset + 6] = static_cast<std::byte>(exit_code);
    image[code_offset + 10] = std::byte{0x0f}; image[code_offset + 11] = std::byte{0x05};
    return image;
}

std::vector<std::byte>
elf_writer::emit_write_exit_executable(std::span<const std::byte> payload, std::uint8_t exit_code) {
    if (payload.empty()) return emit_exit_executable(exit_code);
    const write_call call{payload, 1};
    return emit_multi_write_exit_executable(std::span<const write_call>{&call, 1}, exit_code);
}

namespace {
constexpr std::size_t per_write_code_size = 24;
constexpr std::size_t exit_code_size = 12;
constexpr std::size_t header_size = 64;
constexpr std::size_t program_header_size = 56;
constexpr std::size_t code_offset = header_size + program_header_size;

void emit_elf_header(std::vector<std::byte>& image, std::size_t code_size) {
    constexpr std::uint64_t image_base = 0x400000;
    image[0] = std::byte{0x7f}; image[1] = std::byte{'E'}; image[2] = std::byte{'L'}; image[3] = std::byte{'F'};
    image[4] = std::byte{2}; image[5] = std::byte{1}; image[6] = std::byte{1};
    write16(image, 16, 2); write16(image, 18, 62); write32(image, 20, 1);
    write64(image, 24, image_base + code_offset); write64(image, 32, header_size);
    write16(image, 52, header_size); write16(image, 54, program_header_size); write16(image, 56, 1);
    write32(image, 64, 1); write32(image, 68, 5); write64(image, 72, 0);
    write64(image, 80, image_base); write64(image, 88, image_base);
    write64(image, 96, image.size()); write64(image, 104, image.size()); write64(image, 112, 0x1000);
    (void)code_size;
}

void emit_write_block(std::vector<std::byte>& image, std::size_t code_offset, std::size_t payload_offset,
                      const write_call& call) {
    const auto fd = call.file_descriptor;
    const auto size = call.payload.size();
    const auto displacement = static_cast<std::int32_t>(payload_offset) - static_cast<std::int32_t>(code_offset + 17);
    const std::array<std::byte, per_write_code_size> code = {
        std::byte{0xb8}, std::byte{1}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0xbf}, static_cast<std::byte>(fd & 0xffU), static_cast<std::byte>((fd >> 8U) & 0xffU),
        static_cast<std::byte>((fd >> 16U) & 0xffU), static_cast<std::byte>((fd >> 24U) & 0xffU),
        std::byte{0x48}, std::byte{0x8d}, std::byte{0x35},
        static_cast<std::byte>(displacement & 0xffU), static_cast<std::byte>((displacement >> 8U) & 0xffU),
        static_cast<std::byte>((displacement >> 16U) & 0xffU), static_cast<std::byte>((displacement >> 24U) & 0xffU),
        std::byte{0xba}, static_cast<std::byte>(size & 0xffU), static_cast<std::byte>((size >> 8U) & 0xffU),
        static_cast<std::byte>((size >> 16U) & 0xffU), static_cast<std::byte>((size >> 24U) & 0xffU),
        std::byte{0x0f}, std::byte{0x05},
    };
    std::copy(code.begin(), code.end(), image.begin() + static_cast<std::ptrdiff_t>(code_offset));
    std::copy(call.payload.begin(), call.payload.end(), image.begin() + static_cast<std::ptrdiff_t>(payload_offset));
}
}

std::vector<std::byte>
elf_writer::emit_multi_write_exit_executable(std::span<const write_call> writes, std::uint8_t exit_code) {
    return emit_syscall_program_executable(writes, {}, exit_code);
}

namespace {
constexpr std::size_t per_read_code_size = 24;

void emit_read_block(std::vector<std::byte>& image, std::size_t code_offset, std::size_t buffer_offset,
                     const read_call& call) {
    const auto fd = call.file_descriptor;
    const auto size = call.max_bytes;
    const auto displacement = static_cast<std::int32_t>(buffer_offset) - static_cast<std::int32_t>(code_offset + 17);
    const std::array<std::byte, per_read_code_size> code = {
        std::byte{0xb8}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0xbf}, static_cast<std::byte>(fd & 0xffU), static_cast<std::byte>((fd >> 8U) & 0xffU),
        static_cast<std::byte>((fd >> 16U) & 0xffU), static_cast<std::byte>((fd >> 24U) & 0xffU),
        std::byte{0x48}, std::byte{0x8d}, std::byte{0x35},
        static_cast<std::byte>(displacement & 0xffU), static_cast<std::byte>((displacement >> 8U) & 0xffU),
        static_cast<std::byte>((displacement >> 16U) & 0xffU), static_cast<std::byte>((displacement >> 24U) & 0xffU),
        std::byte{0xba}, static_cast<std::byte>(size & 0xffU), static_cast<std::byte>((size >> 8U) & 0xffU),
        static_cast<std::byte>((size >> 16U) & 0xffU), static_cast<std::byte>((size >> 24U) & 0xffU),
        std::byte{0x0f}, std::byte{0x05},
    };
    std::copy(code.begin(), code.end(), image.begin() + static_cast<std::ptrdiff_t>(code_offset));
}
}

std::vector<std::byte>
elf_writer::emit_syscall_program_executable(std::span<const write_call> writes,
                                           std::span<const read_call> reads,
                                           std::uint8_t exit_code) {
    const auto code_size = per_write_code_size * writes.size() + per_read_code_size * reads.size() + exit_code_size;
    const auto payload_start = code_offset + code_size;
    std::size_t total_payload = 0;
    for (const auto& call : writes) total_payload += call.payload.size();
    for (const auto& call : reads) total_payload += call.max_bytes;
    std::vector<std::byte> image(payload_start + total_payload);
    emit_elf_header(image, code_size);
    std::size_t current_code = code_offset;
    std::size_t current_payload = payload_start;
    for (const auto& call : writes) {
        emit_write_block(image, current_code, current_payload, call);
        current_code += per_write_code_size;
        current_payload += call.payload.size();
    }
    for (const auto& call : reads) {
        emit_read_block(image, current_code, current_payload, call);
        current_code += per_read_code_size;
        current_payload += call.max_bytes;
    }
    const std::array<std::byte, exit_code_size> code = {
        std::byte{0xb8}, std::byte{0x3c}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0xbf}, static_cast<std::byte>(exit_code), std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0x0f}, std::byte{0x05},
    };
    std::copy(code.begin(), code.end(), image.begin() + static_cast<std::ptrdiff_t>(current_code));
    return image;
}

namespace {
constexpr std::size_t per_open_code_size = 24;
constexpr std::size_t per_close_code_size = 12;
constexpr std::size_t fd_move_code_size = 3;

void emit_open_block(std::vector<std::byte>& image, std::size_t code_offset, std::size_t path_offset,
                     const open_call& call) {
    const auto displacement = static_cast<std::int32_t>(path_offset) - static_cast<std::int32_t>(code_offset + 12);
    const std::array<std::byte, per_open_code_size> code = {
        std::byte{0xb8}, std::byte{2}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0x48}, std::byte{0x8d}, std::byte{0x3d},
        static_cast<std::byte>(displacement & 0xffU), static_cast<std::byte>((displacement >> 8U) & 0xffU),
        static_cast<std::byte>((displacement >> 16U) & 0xffU), static_cast<std::byte>((displacement >> 24U) & 0xffU),
        std::byte{0xbe}, static_cast<std::byte>(call.flags & 0xffU), static_cast<std::byte>((call.flags >> 8U) & 0xffU),
        static_cast<std::byte>((call.flags >> 16U) & 0xffU), static_cast<std::byte>((call.flags >> 24U) & 0xffU),
        std::byte{0xba}, static_cast<std::byte>(call.mode & 0xffU), static_cast<std::byte>((call.mode >> 8U) & 0xffU),
        static_cast<std::byte>((call.mode >> 16U) & 0xffU), static_cast<std::byte>((call.mode >> 24U) & 0xffU),
        std::byte{0x0f}, std::byte{0x05},
    };
    std::copy(code.begin(), code.end(), image.begin() + static_cast<std::ptrdiff_t>(code_offset));
    std::copy(call.filename.begin(), call.filename.end(), image.begin() + static_cast<std::ptrdiff_t>(path_offset));
    image[static_cast<std::ptrdiff_t>(path_offset) + static_cast<std::ptrdiff_t>(call.filename.size())] = std::byte{0};
}

void emit_close_block(std::vector<std::byte>& image, std::size_t code_offset, const close_call& call) {
    const std::array<std::byte, per_close_code_size> code = {
        std::byte{0xb8}, std::byte{3}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0xbf}, static_cast<std::byte>(call.file_descriptor & 0xffU),
        static_cast<std::byte>((call.file_descriptor >> 8U) & 0xffU),
        static_cast<std::byte>((call.file_descriptor >> 16U) & 0xffU),
        static_cast<std::byte>((call.file_descriptor >> 24U) & 0xffU),
        std::byte{0x0f}, std::byte{0x05},
    };
    std::copy(code.begin(), code.end(), image.begin() + static_cast<std::ptrdiff_t>(code_offset));
}

void emit_fd_move(std::vector<std::byte>& image, std::size_t code_offset) {
    const std::array<std::byte, fd_move_code_size> code = {
        std::byte{0x48}, std::byte{0x89}, std::byte{0xc7},
    };
    std::copy(code.begin(), code.end(), image.begin() + static_cast<std::ptrdiff_t>(code_offset));
}

std::size_t action_payload_size(const syscall_action& action) {
    switch (action.kind) {
    case syscall_action::kind::write: return action.write.payload.size();
    case syscall_action::kind::read: return action.read.max_bytes;
    case syscall_action::kind::open: return action.open.filename.size() + 1;
    case syscall_action::kind::close: return 0;
    }
    return 0;
}

std::size_t action_code_size(const syscall_action& action, bool emit_fd_move_after) {
    std::size_t base = 0;
    switch (action.kind) {
    case syscall_action::kind::write: base = per_write_code_size; break;
    case syscall_action::kind::read: base = per_read_code_size; break;
    case syscall_action::kind::open: base = per_open_code_size; break;
    case syscall_action::kind::close: base = per_close_code_size; break;
    }
    if (emit_fd_move_after && (action.kind == syscall_action::kind::open)) base += fd_move_code_size;
    return base;
}
}

std::vector<std::byte>
elf_writer::emit_action_program_executable(std::span<const syscall_action> actions, std::uint8_t exit_code) {
    std::size_t code_size = exit_code_size;
    std::size_t total_payload = 0;
    for (std::size_t index = 0; index < actions.size(); ++index) {
        const bool needs_fd_move = actions[index].kind == syscall_action::kind::open;
        code_size += action_code_size(actions[index], needs_fd_move);
        total_payload += action_payload_size(actions[index]);
    }
    const auto payload_start = code_offset + code_size;
    std::vector<std::byte> image(payload_start + total_payload);
    emit_elf_header(image, code_size);
    std::size_t current_code = code_offset;
    std::size_t current_payload = payload_start;
    for (std::size_t index = 0; index < actions.size(); ++index) {
        const auto& action = actions[index];
        const bool needs_fd_move = action.kind == syscall_action::kind::open;
        switch (action.kind) {
        case syscall_action::kind::write:
            emit_write_block(image, current_code, current_payload, action.write);
            current_payload += action.write.payload.size();
            break;
        case syscall_action::kind::read:
            emit_read_block(image, current_code, current_payload, action.read);
            current_payload += action.read.max_bytes;
            break;
        case syscall_action::kind::open:
            emit_open_block(image, current_code, current_payload, action.open);
            current_payload += action.open.filename.size() + 1;
            break;
        case syscall_action::kind::close:
            emit_close_block(image, current_code, action.close);
            break;
        }
        current_code += action_code_size(action, false);
        if (needs_fd_move) {
            emit_fd_move(image, current_code);
            current_code += fd_move_code_size;
        }
    }
    const std::array<std::byte, exit_code_size> code = {
        std::byte{0xb8}, std::byte{0x3c}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0xbf}, static_cast<std::byte>(exit_code), std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0x0f}, std::byte{0x05},
    };
    std::copy(code.begin(), code.end(), image.begin() + static_cast<std::ptrdiff_t>(current_code));
    return image;
}

} // namespace dosrecomp::backend
