#include "dosrecomp/compiler/branching_compiler.hpp"

#include "dosrecomp/cfg/cfg_builder.hpp"
#include "dosrecomp/compiler/straight_line_compiler.hpp"
#include "dosrecomp/decoder/instruction_decoder.hpp"
#include "dosrecomp/loader/binary_loader.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace dosrecomp::compiler {
namespace {

constexpr std::size_t dos_memory_size = 0x100000;

constexpr std::size_t word_reg_index(decoder::register_name r) {
    if (r >= decoder::register_name::ax && r <= decoder::register_name::di) {
        return static_cast<std::size_t>(r) - static_cast<std::size_t>(decoder::register_name::ax);
    }
    return static_cast<std::size_t>(r);
}

[[nodiscard]] std::string word_reg_access(decoder::register_name r) {
    std::ostringstream out;
    out << "regs[" << word_reg_index(r) << "]";
    return out.str();
}

[[nodiscard]] std::string byte_reg_access(decoder::register_name r) {
    int shift = 0;
    decoder::register_name word_reg = decoder::register_name::ax;
    switch (r) {
    case decoder::register_name::al: shift = 0; word_reg = decoder::register_name::ax; break;
    case decoder::register_name::ah: shift = 8; word_reg = decoder::register_name::ax; break;
    case decoder::register_name::cl: shift = 0; word_reg = decoder::register_name::cx; break;
    case decoder::register_name::ch: shift = 8; word_reg = decoder::register_name::cx; break;
    case decoder::register_name::dl: shift = 0; word_reg = decoder::register_name::dx; break;
    case decoder::register_name::dh: shift = 8; word_reg = decoder::register_name::dx; break;
    case decoder::register_name::bl: shift = 0; word_reg = decoder::register_name::bx; break;
    case decoder::register_name::bh: shift = 8; word_reg = decoder::register_name::bx; break;
    default: shift = 0; word_reg = r; break;
    }
    std::ostringstream out;
    out << "static_cast<uint8_t>(regs[" << word_reg_index(word_reg) << "] >> " << shift << ")";
    return out.str();
}

[[nodiscard]] std::string byte_reg_store(decoder::register_name r, const std::string& value_expr) {
    int shift = 0;
    decoder::register_name word_reg = decoder::register_name::ax;
    switch (r) {
    case decoder::register_name::al: shift = 0; word_reg = decoder::register_name::ax; break;
    case decoder::register_name::ah: shift = 8; word_reg = decoder::register_name::ax; break;
    case decoder::register_name::cl: shift = 0; word_reg = decoder::register_name::cx; break;
    case decoder::register_name::ch: shift = 8; word_reg = decoder::register_name::cx; break;
    case decoder::register_name::dl: shift = 0; word_reg = decoder::register_name::dx; break;
    case decoder::register_name::dh: shift = 8; word_reg = decoder::register_name::dx; break;
    case decoder::register_name::bl: shift = 0; word_reg = decoder::register_name::bx; break;
    case decoder::register_name::bh: shift = 8; word_reg = decoder::register_name::bx; break;
    default: return "/* unsupported byte store */";
    }
    std::ostringstream out;
    out << "regs[" << word_reg_index(word_reg) << "] = static_cast<uint16_t>((regs["
        << word_reg_index(word_reg) << "] & static_cast<uint16_t>(~((uint16_t)0xffu << " << shift
        << "))) | (static_cast<uint16_t>(" << value_expr << ") << " << shift << "))";
    return out.str();
}

[[nodiscard]] std::string block_label(std::size_t offset) {
    std::ostringstream out;
    out << "block_0x" << std::hex << offset;
    return out.str();
}

void emit_runtime_header(std::ostream& out) {
    out << "#include <cstdint>\n";
    out << "#include <cstdio>\n";
    out << "#include <cstdlib>\n";
    out << "#include <fcntl.h>\n";
    out << "#include <sys/syscall.h>\n";
    out << "#include <unistd.h>\n";
    out << "\n";
    out << "static uint16_t regs[20] = {0};\n";
    out << "static uint8_t mem[" << std::hex << dos_memory_size << std::dec << "] = {0};\n";
    out << "static uint32_t flags = 0;\n";
    out << "static int next_fake_handle = 3;\n";
    out << "static int handle_to_fd[16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};\n";
    out << "\n";
    out << "static inline void update_flags_cmp(uint16_t a, uint16_t b, bool is_byte) {\n";
    out << "    uint16_t r;\n";
    out << "    if (is_byte) {\n";
    out << "        const uint8_t aa = static_cast<uint8_t>(a);\n";
    out << "        const uint8_t bb = static_cast<uint8_t>(b);\n";
    out << "        r = static_cast<uint16_t>(aa - bb);\n";
    out << "        flags = (aa == bb ? 0x40u : 0u)\n";
    out << "             | ((r & 0x80) ? 0x80u : 0u)\n";
    out << "             | (aa < bb ? 1u : 0u);\n";
    out << "    } else {\n";
    out << "        r = static_cast<uint16_t>(a - b);\n";
    out << "        flags = (a == b ? 0x40u : 0u)\n";
    out << "             | ((r & 0x8000) ? 0x8000u : 0u)\n";
    out << "             | (a < b ? 1u : 0u);\n";
    out << "    }\n";
    out << "}\n";
    out << "\n";
}

void emit_int21_runtime(std::ostream& out) {
    out << "  {\n";
    out << "    const uint16_t __ax = regs[0];\n";
    out << "    const uint8_t ah = static_cast<uint8_t>(__ax >> 8);\n";
    out << "    const uint8_t al = static_cast<uint8_t>(__ax & 0xff);\n";
    out << "    switch (ah) {\n";
    out << "    case 0x4c: syscall(SYS_exit, __ax & 0xff); return;\n";
    out << "    case 0x02: {\n";
    out << "      uint8_t ch = static_cast<uint8_t>(regs[2]);\n";
    out << "      syscall(SYS_write, 1, &ch, 1);\n";
    out << "      break;\n";
    out << "    }\n";
    out << "    case 0x09: {\n";
    out << "      uint32_t base = (static_cast<uint32_t>(regs[11]) << 4) + regs[2];\n";
    out << "      uint32_t len = 0; while (mem[base + len] != '$') ++len;\n";
    out << "      syscall(SYS_write, 1, &mem[base], len);\n";
    out << "      break;\n";
    out << "    }\n";
    out << "    case 0x01: {\n";
    out << "      char ch = 0; long r = syscall(SYS_read, 0, &ch, 1);\n";
    out << "      if (r <= 0) { syscall(SYS_exit, 1); return; }\n";
    out << "      regs[0] = (regs[0] & 0xff00) | static_cast<uint16_t>(static_cast<uint8_t>(ch));\n";
    out << "      break;\n";
    out << "    }\n";
    out << "    case 0x0a: {\n";
    out << "      uint32_t base = (static_cast<uint32_t>(regs[11]) << 4) + regs[2];\n";
    out << "      uint8_t cap = mem[base];\n";
    out << "      uint8_t idx = 1;\n";
    out << "      while (idx <= cap) {\n";
    out << "        char ch = 0; long r = syscall(SYS_read, 0, &ch, 1);\n";
    out << "        if (r <= 0) break;\n";
    out << "        if (ch == '\\n') break;\n";
    out << "        mem[base + idx] = static_cast<uint8_t>(ch); ++idx;\n";
    out << "      }\n";
    out << "      mem[base + 1] = idx - 1;\n";
    out << "      regs[0] = idx - 1;\n";
    out << "      break;\n";
    out << "    }\n";
    out << "    case 0x3d: {\n";
    out << "      uint32_t base = (static_cast<uint32_t>(regs[11]) << 4) + regs[2];\n";
    out << "      uint32_t len = 0; while (mem[base + len] != 0) ++len;\n";
    out << "      int oflags = 0;\n";
    out << "      switch (al) {\n";
    out << "        case 0: oflags = O_RDONLY; break;\n";
    out << "        case 1: oflags = O_WRONLY | O_CREAT | O_TRUNC; break;\n";
    out << "        case 2: oflags = O_RDWR | O_CREAT; break;\n";
    out << "        default: syscall(SYS_exit, 1); return;\n";
    out << "      }\n";
    out << "      int fd = open(reinterpret_cast<const char*>(&mem[base]), oflags, 0644);\n";
    out << "      int handle = next_fake_handle++;\n";
    out << "      if (handle < 16) handle_to_fd[handle] = fd;\n";
    out << "      regs[0] = handle; regs[3] = handle;\n";
    out << "      break;\n";
    out << "    }\n";
    out << "    case 0x3e: {\n";
    out << "      int h = regs[3] & 0xff;\n";
    out << "      if (h >= 3 && h < 16 && handle_to_fd[h] >= 0) {\n";
    out << "        close(handle_to_fd[h]); handle_to_fd[h] = -1;\n";
    out << "      }\n";
    out << "      break;\n";
    out << "    }\n";
    out << "    case 0x3f: {\n";
    out << "      int h = regs[3] & 0xff;\n";
    out << "      uint16_t cnt = regs[1];\n";
    out << "      uint32_t base = (static_cast<uint32_t>(regs[11]) << 4) + regs[2];\n";
    out << "      int src = (h >= 3 && h < 16) ? handle_to_fd[h] : 0;\n";
    out << "      long r = (cnt > 0 && src >= 0) ? syscall(SYS_read, src, &mem[base], cnt) : 0;\n";
    out << "      regs[0] = (r > 0) ? static_cast<uint16_t>(r) : 0;\n";
    out << "      break;\n";
    out << "    }\n";
    out << "    case 0x40: {\n";
    out << "      int h = regs[3] & 0xff;\n";
    out << "      uint16_t cnt = regs[1];\n";
    out << "      uint32_t base = (static_cast<uint32_t>(regs[11]) << 4) + regs[2];\n";
    out << "      int dst = (h == 1) ? 1 : (h == 2) ? 2 : (h >= 3 && h < 16) ? handle_to_fd[h] : 1;\n";
    out << "      if (cnt > 0) syscall(SYS_write, dst, &mem[base], cnt);\n";
    out << "      break;\n";
    out << "    }\n";
    out << "    default: syscall(SYS_exit, 1); return;\n";
    out << "    }\n";
    out << "  }\n";
}

void emit_int10_runtime(std::ostream& out) {
    out << "  {\n";
    out << "    const uint8_t ah = static_cast<uint8_t>(regs[0] >> 8);\n";
    out << "    switch (ah) {\n";
    out << "    case 0x0e: {\n";
    out << "      uint8_t ch = static_cast<uint8_t>(regs[0]);\n";
    out << "      syscall(SYS_write, 1, &ch, 1);\n";
    out << "      break;\n";
    out << "    }\n";
    out << "    case 0x02: break;\n";
    out << "    default: syscall(SYS_exit, 1); return;\n";
    out << "    }\n";
    out << "  }\n";
}

void emit_move_immediate(std::ostream& out, const decoder::instruction& decoded) {
    const auto& dst = decoded.operands[0];
    const auto width = dst.width;
    const auto imm = decoded.operands[1].immediate;
    if (width == decoder::operand_width::byte) {
        std::ostringstream imm_str; imm_str << "0x" << std::hex << (imm & 0xffU) << std::dec;
        out << "  " << byte_reg_store(dst.reg, imm_str.str()) << ";\n";
    } else {
        out << "  " << word_reg_access(dst.reg) << " = 0x" << std::hex << imm << std::dec << ";\n";
    }
}

void emit_arithmetic(std::ostream& out, const decoder::instruction& decoded) {
    const auto& dst = decoded.operands[0];
    const auto width = dst.width;
    const auto is_cmp = decoded.alu == decoder::alu_operation::compare;
    const auto is_test = decoded.alu == decoder::alu_operation::test;
    const auto is_byte = width == decoder::operand_width::byte;
    std::string op;
    switch (decoded.alu) {
    case decoder::alu_operation::add: op = "+="; break;
    case decoder::alu_operation::subtract: op = "-="; break;
    case decoder::alu_operation::bit_and: op = "&="; break;
    case decoder::alu_operation::bit_or: op = "|="; break;
    case decoder::alu_operation::bit_xor: op = "^="; break;
    default: break;
    }
    const auto dst_acc = is_byte ? byte_reg_access(dst.reg) : word_reg_access(dst.reg);
    if (is_cmp || is_test) {
        out << "  update_flags_cmp(" << dst_acc << ",";
        if (decoded.operands[1].kind == decoder::operand_kind::immediate) {
            out << "0x" << std::hex << decoded.operands[1].immediate << std::dec;
        } else {
            const auto& src = decoded.operands[1];
            out << (src.width == decoder::operand_width::byte ? byte_reg_access(src.reg) : word_reg_access(src.reg));
        }
        out << "," << (is_byte ? "true" : "false") << ");\n";
        return;
    }
    out << "  " << word_reg_access(dst.reg) << " " << op << " 0x"
        << std::hex << decoded.operands[1].immediate << std::dec << ";\n";
    out << "  update_flags_cmp(" << word_reg_access(dst.reg) << ", 0, false);\n";
}

[[nodiscard]] bool emit_conditional(std::ostream& out, decoder::branch_condition cond) {
    switch (cond) {
    case decoder::branch_condition::equal: out << "(flags & 0x40u)"; return true;
    case decoder::branch_condition::not_equal: out << "!(flags & 0x40u)"; return true;
    case decoder::branch_condition::below: out << "(flags & 0x1u)"; return true;
    case decoder::branch_condition::above_or_equal: out << "!(flags & 0x1u)"; return true;
    case decoder::branch_condition::below_or_equal: out << "((flags & 0x1u) || (flags & 0x40u))"; return true;
    case decoder::branch_condition::above: out << "(!(flags & 0x1u) && !(flags & 0x40u))"; return true;
    case decoder::branch_condition::sign: out << "(flags & 0x8000u)"; return true;
    case decoder::branch_condition::not_sign: out << "!(flags & 0x8000u)"; return true;
    case decoder::branch_condition::less: out << "((bool)(flags & 0x8000u) != (bool)(flags & 0x40u))"; return true;
    case decoder::branch_condition::greater_or_equal: out << "((bool)(flags & 0x8000u) == (bool)(flags & 0x40u))"; return true;
    case decoder::branch_condition::less_or_equal: out << "((flags & 0x40u) || (bool)(flags & 0x8000u) != (bool)(flags & 0x40u))"; return true;
    case decoder::branch_condition::greater: out << "(!(flags & 0x40u) && (bool)(flags & 0x8000u) == (bool)(flags & 0x40u))"; return true;
    case decoder::branch_condition::overflow: out << "((flags & 0x8000u) != 0)"; return true;
    case decoder::branch_condition::not_overflow: out << "((flags & 0x8000u) == 0)"; return true;
    default: return false;
    }
}

[[nodiscard]] std::expected<void, branching_compile_error>
emit_block_at(std::ostream& out, const loader::program_image& image, std::size_t start) {
    out << "static void " << block_label(start) << "() {\n";
    std::size_t position = start;
    while (position < image.bytes.size()) {
        const auto decoded = decoder::instruction_decoder::decode_at(image.bytes, position);
        if (!decoded) return std::unexpected(branching_compile_error{std::string{"cannot decode at "} + std::to_string(position) + ": " + decoded.error().message});
        const auto next = position + decoded->size;
        if (decoded->kind == decoder::instruction_kind::interrupt) {
            if (decoded->interrupt_number == 0x21) emit_int21_runtime(out);
            else if (decoded->interrupt_number == 0x10) emit_int10_runtime(out);
            else { out << "  syscall(SYS_exit, 1); return;\n"; }
            position = next;
            continue;
        }
        if (decoded->kind == decoder::instruction_kind::jump) {
            const auto target = static_cast<std::size_t>(position + decoded->size + decoded->relative_target);
            out << "  " << block_label(target) << "();\n";
            position = next;
            break;
        }
        if (decoded->kind == decoder::instruction_kind::conditional_jump) {
            const auto target = static_cast<std::size_t>(position + decoded->size + decoded->relative_target);
            out << "  if (";
            if (!emit_conditional(out, decoded->condition)) {
                out << "/* unsupported cond */ false";
            }
            out << ") " << block_label(target) << "(); else " << block_label(next) << "();\n";
            position = next;
            break;
        }
        if (decoded->kind == decoder::instruction_kind::return_) {
            out << "  syscall(SYS_exit, regs[0] & 0xff);\n";
            out << "  return;\n";
            position = next;
            break;
        }
        if (decoded->kind == decoder::instruction_kind::call) {
            out << "  // CALL not yet supported in runtime branches\n  syscall(SYS_exit, 1); return;\n";
            position = next;
            break;
        }
        switch (decoded->kind) {
        case decoder::instruction_kind::move_immediate:
            emit_move_immediate(out, *decoded);
            break;
        case decoder::instruction_kind::arithmetic:
        case decoder::instruction_kind::compare:
            emit_arithmetic(out, *decoded);
            break;
        default:
            out << "  // unsupported instruction at 0x" << std::hex << position << std::dec << "\n";
            out << "  syscall(SYS_exit, 1); return;\n";
            position = next;
            break;
        }
        position = next;
    }
    out << "}\n\n";
    return {};
}

[[nodiscard]] std::expected<std::set<std::size_t>, branching_compile_error>
collect_block_starts(const loader::program_image& image) {
    std::set<std::size_t> starts;
    starts.insert(image.entry_offset());
    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<std::size_t> current(starts.begin(), starts.end());
        for (const auto start : current) {
            std::size_t position = start;
            while (position < image.bytes.size()) {
                const auto decoded = decoder::instruction_decoder::decode_at(image.bytes, position);
                if (!decoded) return std::unexpected(branching_compile_error{std::string{"cannot decode at "} + std::to_string(position) + ": " + decoded.error().message});
                const auto next = position + decoded->size;
                if (decoded->kind == decoder::instruction_kind::jump ||
                    decoded->kind == decoder::instruction_kind::conditional_jump) {
                    const auto target = static_cast<std::size_t>(position + decoded->size + decoded->relative_target);
                    if (starts.insert(target).second) changed = true;
                }
                if (decoded->kind == decoder::instruction_kind::conditional_jump ||
                    decoded->kind == decoder::instruction_kind::call ||
                    decoded->kind == decoder::instruction_kind::interrupt) {
                    if (next < image.bytes.size()) {
                        if (starts.insert(next).second) changed = true;
                    }
                }
                if (decoded->kind == decoder::instruction_kind::jump ||
                    decoded->kind == decoder::instruction_kind::conditional_jump ||
                    decoded->kind == decoder::instruction_kind::return_ ||
                    decoded->kind == decoder::instruction_kind::interrupt ||
                    decoded->kind == decoder::instruction_kind::call) {
                    break;
                }
                position = next;
            }
        }
    }
    return starts;
}

[[nodiscard]] std::expected<void, branching_compile_error>
write_main(std::ostream& out, const loader::program_image& image) {
    out << "int main() {\n";
    out << "  regs[4] = 0xfffe;\n";
    if (image.format == loader::executable_format::mz) {
        const auto base = static_cast<std::uint32_t>(image.entry_point.segment) * 16U;
        out << "  static const uint8_t img[] = {";
        for (std::size_t i = 0; i < image.bytes.size(); ++i) {
            if (i % 16 == 0) out << "\n    ";
            out << "0x" << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<unsigned>(std::to_integer<std::uint8_t>(image.bytes[i])) << std::dec << ",";
        }
        out << "\n  };\n";
        out << "  for (size_t i = 0; i < sizeof(img); ++i) mem[" << std::hex << base << " + i] = img[i];\n" << std::dec;
        out << "  regs[10] = " << image.initial_stack.segment << ";\n";
        out << "  regs[4] = " << image.initial_stack.offset << ";\n";
    } else {
        out << "  static const uint8_t img[] = {";
        for (std::size_t i = 0; i < image.bytes.size(); ++i) {
            if (i % 16 == 0) out << "\n    ";
            out << "0x" << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<unsigned>(std::to_integer<std::uint8_t>(image.bytes[i])) << std::dec << ",";
        }
        out << "\n  };\n";
        out << "  for (size_t i = 0; i < sizeof(img); ++i) mem[0x100 + i] = img[i];\n";
    }
    out << "  " << block_label(image.entry_offset()) << "();\n";
    out << "  return 0;\n";
    out << "}\n";
    return {};
}

[[nodiscard]] std::expected<std::string, branching_compile_error>
emit_c_runtime(const loader::program_image& image) {
    const auto starts = collect_block_starts(image);
    if (!starts) return std::unexpected(starts.error());
    std::ostringstream out;
    emit_runtime_header(out);
    for (const auto start : *starts) {
        out << "static void " << block_label(start) << "();\n";
    }
    out << "\n";
    for (const auto start : *starts) {
        const auto result = emit_block_at(out, image, start);
        if (!result) return std::unexpected(result.error());
    }
    const auto main_result = write_main(out, image);
    if (!main_result) return std::unexpected(main_result.error());
    return out.str();
}

} // namespace

std::expected<std::uint8_t, branching_compile_error>
branching_compiler::extract_exit_code(const loader::program_image& image) {
    const auto result = straight_line_compiler::extract_exit_code(image);
    if (!result) return std::unexpected(branching_compile_error{result.error().message});
    return *result;
}

std::expected<std::vector<std::byte>, branching_compile_error>
branching_compiler::compile(const loader::program_image& image) {
    const auto c_source = emit_c_runtime(image);
    if (!c_source) return std::unexpected(c_source.error());
    const auto tmp_dir = std::filesystem::temp_directory_path();
    const auto c_path = tmp_dir / "dosrecomp_branching.cpp";
    const auto out_path = tmp_dir / "dosrecomp_branching.elf";
    std::error_code ec;
    std::filesystem::remove(out_path, ec);
    {
        std::ofstream c_file(c_path, std::ios::binary | std::ios::trunc);
        if (!c_file) return std::unexpected(branching_compile_error{"cannot write " + c_path.string()});
        c_file.write(c_source->data(), static_cast<std::streamsize>(c_source->size()));
    }
    const std::string cmd = "g++ -O2 -std=c++23 -static -o " + out_path.string() + " " + c_path.string() + " 2>&1";
    if (std::system(cmd.c_str()) != 0) return std::unexpected(branching_compile_error{"g++ failed to compile runtime C output (see above)"});
    std::ifstream binary(out_path, std::ios::binary | std::ios::ate);
    if (!binary) return std::unexpected(branching_compile_error{"cannot reopen compiled ELF " + out_path.string()});
    const auto size = binary.tellg();
    binary.seekg(0);
    std::vector<std::byte> data(static_cast<std::size_t>(size));
    if (!binary.read(reinterpret_cast<char*>(data.data()), size)) return std::unexpected(branching_compile_error{"cannot read compiled ELF"});
    return data;
}

std::expected<std::string, branching_compile_error>
branching_compiler::emit_llvm(const loader::program_image& image) {
    const auto result = straight_line_compiler::emit_llvm(image);
    if (!result) return std::unexpected(branching_compile_error{result.error().message});
    return *result;
}

} // namespace dosrecomp::compiler