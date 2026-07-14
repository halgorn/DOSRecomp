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
    return "regs[" + std::to_string(word_reg_index(r)) + "]";
}

[[nodiscard]] std::pair<int, decoder::register_name> byte_reg_locus(decoder::register_name r) {
    switch (r) {
    case decoder::register_name::al: return {0, decoder::register_name::ax};
    case decoder::register_name::ah: return {8, decoder::register_name::ax};
    case decoder::register_name::cl: return {0, decoder::register_name::cx};
    case decoder::register_name::ch: return {8, decoder::register_name::cx};
    case decoder::register_name::dl: return {0, decoder::register_name::dx};
    case decoder::register_name::dh: return {8, decoder::register_name::dx};
    case decoder::register_name::bl: return {0, decoder::register_name::bx};
    case decoder::register_name::bh: return {8, decoder::register_name::bx};
    default: return {0, r};
    }
}

[[nodiscard]] std::string byte_reg_access(decoder::register_name r) {
    const auto [shift, word_reg] = byte_reg_locus(r);
    return "static_cast<uint8_t>(regs[" + std::to_string(word_reg_index(word_reg)) + "] >> " + std::to_string(shift) + ")";
}

[[nodiscard]] std::string byte_reg_store(decoder::register_name r, const std::string& value_expr) {
    const auto [shift, word_reg] = byte_reg_locus(r);
    const auto idx = std::to_string(word_reg_index(word_reg));
    return "regs[" + idx + "] = static_cast<uint16_t>((regs[" + idx + "] & static_cast<uint16_t>(~((uint16_t)0xffu << "
        + std::to_string(shift) + "))) | (static_cast<uint16_t>(" + value_expr + ") << " + std::to_string(shift) + "))";
}

[[nodiscard]] std::string block_label(std::size_t offset) {
    std::ostringstream out;
    out << "block_0x" << std::hex << offset;
    return out.str();
}

void emit_runtime_header(std::ostream& out) {
    out <<
        "#include <cstdint>\n#include <cstdio>\n#include <cstdlib>\n#include <ctime>\n"
        "#include <fcntl.h>\n#include <sys/syscall.h>\n#include <unistd.h>\n"
        "static uint16_t regs[20] = {0}; static uint8_t mem[" << std::hex << dos_memory_size << std::dec << "] = {0};\n"
        "static uint32_t flags = 0; static int next_fake_handle = 3;\n"
        "static int handle_to_fd[16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};\n"
        "static uint16_t call_stack[256] = {0}; static int call_sp = 0;\n"
        "static inline void update_flags_cmp(uint16_t a, uint16_t b, bool is_byte) {\n"
        "    if (is_byte) { uint8_t aa = (uint8_t)a, bb = (uint8_t)b, r = (uint8_t)(aa - bb);\n"
        "      flags = (aa == bb ? 0x40u : 0u) | ((r & 0x80) ? 0x80u : 0u) | (aa < bb ? 1u : 0u); }\n"
        "    else { uint16_t r = (uint16_t)(a - b);\n"
        "      flags = (a == b ? 0x40u : 0u) | ((r & 0x8000) ? 0x8000u : 0u) | (a < b ? 1u : 0u); }\n}\n"
        "static inline __attribute__((always_inline)) int df_step(int d) { return (flags & 0x400u) ? -d : d; }\n"
        "static void dispatch(uint32_t target);\n";
}

void emit_int21_runtime(std::ostream& out) {
    out <<
        "  { const uint16_t __ax = regs[0]; const uint8_t ah = (uint8_t)(__ax >> 8), al = (uint8_t)__ax;\n"
        "    switch (ah) {\n"
        "    case 0x4c: syscall(SYS_exit, __ax & 0xff); return;\n"
        "    case 0x02: { uint8_t ch = (uint8_t)regs[2]; syscall(SYS_write, 1, &ch, 1); break; }\n"
        "    case 0x09: { uint32_t base = ((uint32_t)regs[11] << 4) + regs[2], len = 0;\n"
        "      while (mem[base + len] != '$') ++len; syscall(SYS_write, 1, &mem[base], len); break; }\n"
        "    case 0x01: { char ch = 0; long r = syscall(SYS_read, 0, &ch, 1);\n"
        "      if (r <= 0) { syscall(SYS_exit, 1); return; }\n"
        "      regs[0] = (regs[0] & 0xff00) | (uint16_t)(uint8_t)ch; break; }\n"
        "    case 0x0a: { uint32_t base = ((uint32_t)regs[11] << 4) + regs[2];\n"
        "      uint8_t cap = mem[base], idx = 1;\n"
        "      while (idx <= cap) { char ch = 0; long r = syscall(SYS_read, 0, &ch, 1);\n"
        "        if (r <= 0 || ch == '\\n') break; mem[base + idx] = (uint8_t)ch; ++idx; }\n"
        "      mem[base + 1] = idx - 1; regs[0] = idx - 1; break; }\n"
        "    case 0x3d: { uint32_t base = ((uint32_t)regs[11] << 4) + regs[2], len = 0;\n"
        "      while (mem[base + len] != 0) ++len; int oflags = 0;\n"
        "      if (al == 0) oflags = O_RDONLY; else if (al == 1) oflags = O_WRONLY | O_CREAT | O_TRUNC;\n"
        "      else if (al == 2) oflags = O_RDWR | O_CREAT; else { syscall(SYS_exit, 1); return; }\n"
        "      int fd = open((const char*)&mem[base], oflags, 0644);\n"
        "      int handle = next_fake_handle++; if (handle < 16) handle_to_fd[handle] = fd;\n"
        "      regs[0] = handle; regs[3] = handle; break; }\n"
        "    case 0x3e: { int h = regs[3] & 0xff;\n"
        "      if (h >= 3 && h < 16 && handle_to_fd[h] >= 0) { close(handle_to_fd[h]); handle_to_fd[h] = -1; } break; }\n"
        "    case 0x3f: { int h = regs[3] & 0xff; uint16_t cnt = regs[1];\n"
        "      uint32_t base = ((uint32_t)regs[11] << 4) + regs[2];\n"
        "      int src = (h >= 3 && h < 16) ? handle_to_fd[h] : 0;\n"
        "      long r = (cnt > 0 && src >= 0) ? syscall(SYS_read, src, &mem[base], cnt) : 0;\n"
        "      regs[0] = (r > 0) ? (uint16_t)r : 0; break; }\n"
        "    case 0x40: { int h = regs[3] & 0xff; uint16_t cnt = regs[1];\n"
        "      uint32_t base = ((uint32_t)regs[11] << 4) + regs[2];\n"
        "      int dst = (h == 1) ? 1 : (h == 2) ? 2 : (h >= 3 && h < 16) ? handle_to_fd[h] : 1;\n"
        "      if (cnt > 0) syscall(SYS_write, dst, &mem[base], cnt); break; }\n"
        "    case 0x2a: case 0x2c: { struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);\n"
        "      struct tm tm; gmtime_r(&ts.tv_sec, &tm);\n"
        "      if (ah == 0x2a) regs[0] = (uint16_t)(((tm.tm_year + 1900 - 1980) << 9) | (tm.tm_mon << 5) | tm.tm_mday), regs[2] = 0;\n"
        "      else regs[0] = (uint16_t)((tm.tm_hour << 8) | tm.tm_min), regs[2] = tm.tm_sec; break; }\n"
        "    case 0x3b: { uint32_t base = ((uint32_t)regs[11] << 4) + regs[2], len = 0;\n"
        "      while (mem[base + len]) ++len; char tmp[260] = {0};\n"
        "      for (uint32_t k = 0; k < len && k < 259; ++k) tmp[k] = (char)mem[base + k];\n"
        "      regs[0] = chdir(tmp) == 0 ? 0u : 0x0005u; break; }\n"
        "    case 0x47: { char tmp[260] = {0};\n"
        "      if (getcwd(tmp, sizeof(tmp) - 1) != nullptr) { uint32_t k = 0;\n"
        "        while (tmp[k] && k < 259) { mem[regs[2] + k] = (uint8_t)tmp[k]; ++k; } regs[0] = 0; }\n"
        "      else regs[0] = 0x0005; break; }\n"
        "    case 0x48: { static uint16_t next_seg = 0x2000; uint16_t want = regs[3];\n"
        "      if (want == 0) { regs[0] = 0x0007; regs[3] = 0xffff; break; }\n"
        "      regs[0] = next_seg; regs[2] = 0; regs[3] = want; next_seg = (uint16_t)(next_seg + want); break; }\n"
        "    case 0x49: regs[0] = 0; regs[3] = 0; break;\n"
        "    case 0x4a: regs[0] = 0; break;\n"
        "    default: syscall(SYS_exit, 1); return;\n    }\n  }\n";
}

void emit_int10_runtime(std::ostream& out) {
    out <<
        "  { const uint8_t ah = (uint8_t)(regs[0] >> 8), al = (uint8_t)regs[0];\n"
        "    uint16_t cx_count = regs[1]; (void)cx_count;\n"
        "    switch (ah) {\n"
        "    case 0x0e: { uint8_t ch = al; syscall(SYS_write, 1, &ch, 1); break; }\n"
        "    case 0x09: { uint8_t ch = al, attr = (uint8_t)regs[3];\n"
        "      for (uint16_t k = 0; k < cx_count; ++k) { syscall(SYS_write, 1, &ch, 1); (void)attr; } break; }\n"
        "    case 0x0a: { uint8_t ch = al;\n"
        "      for (uint16_t k = 0; k < cx_count; ++k) syscall(SYS_write, 1, &ch, 1); break; }\n"
        "    case 0x02: break;\n"
        "    default: syscall(SYS_exit, 1); return;\n    }\n  }\n";
}

void emit_int16_runtime(std::ostream& out) {
    out <<
        "  { const uint8_t ah = (uint8_t)(regs[0] >> 8);\n"
        "    if (ah == 0x00 || ah == 0x10) {\n"
        "      char ch = 0; long r;\n"
        "      do { r = syscall(SYS_read, 0, &ch, 1); } while (r == -1);\n"
        "      if (r < 0) { syscall(SYS_exit, 1); return; }\n"
        "      regs[0] = (uint16_t)(uint8_t)ch; regs[2] = (uint16_t)(uint8_t)ch;\n"
        "    } else { syscall(SYS_exit, 1); return; }\n  }\n";
}

[[nodiscard]] std::string emit_mem_addr(const decoder::operand& mem) {
    if (mem.kind != decoder::operand_kind::memory) return "/* non-memory */";
    bool uses_bp = false;
    for (std::uint8_t i = 0; i < mem.address_register_count; ++i)
        if (mem.address_registers[i] == decoder::register_name::bp) uses_bp = true;
    const auto seg_reg = uses_bp ? decoder::register_name::ss : decoder::register_name::ds;
    const auto seg_index = static_cast<std::size_t>(seg_reg) - static_cast<std::size_t>(decoder::register_name::ax);
    std::ostringstream out;
    out << "((static_cast<uint32_t>(regs[" << seg_index << "]) << 4) + static_cast<uint32_t>(" << mem.displacement;
    for (std::uint8_t i = 0; i < mem.address_register_count; ++i)
        out << " + regs[" << word_reg_index(mem.address_registers[i]) << "]";
    out << "))";
    return out.str();
}
void emit_mem_store(std::ostream& out, const std::string& addr, bool is_byte, const std::string& value) {
    if (is_byte) { out << "  mem[" << addr << "] = " << value << ";\n"; return; }
    out << "  mem[" << addr << "] = static_cast<uint8_t>(" << value << ");\n"
        << "  mem[" << addr << " + 1] = static_cast<uint8_t>((" << value << ") >> 8);\n";
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

void emit_move(std::ostream& out, const decoder::instruction& decoded) {
    const auto& dst = decoded.operands[0];
    const auto& src = decoded.operands[1];
    const auto is_byte = dst.width == decoder::operand_width::byte;
    if (dst.kind == decoder::operand_kind::memory) {
        const auto addr = emit_mem_addr(dst);
        if (src.kind == decoder::operand_kind::reg) emit_mem_store(out, addr, is_byte, is_byte ? byte_reg_access(src.reg) : word_reg_access(src.reg));
        else if (src.kind == decoder::operand_kind::immediate) { std::ostringstream v; v << "0x" << std::hex << src.immediate << std::dec; emit_mem_store(out, addr, is_byte, v.str()); }
        else out << "  // unsupported MOV memory source\n";
        return;
    }
    if (src.kind == decoder::operand_kind::memory) {
        const auto addr = emit_mem_addr(src);
        if (is_byte) { out << "  " << byte_reg_store(dst.reg, "mem[" + addr + "]") << ";\n"; return; }
        out << "  regs[" << word_reg_index(dst.reg) << "] = (uint16_t)mem[" << addr << "] | ((uint16_t)mem["
            << addr << " + 1] << 8);\n"; return;
    }
    if (is_byte) out << "  " << byte_reg_store(dst.reg, byte_reg_access(src.reg)) << ";\n";
    else out << "  regs[" << word_reg_index(dst.reg) << "] = regs[" << word_reg_index(src.reg) << "];\n";
}

void emit_arithmetic(std::ostream& out, const decoder::instruction& decoded) {
    const auto& dst = decoded.operands[0];
    const auto is_cmp = decoded.alu == decoder::alu_operation::compare;
    const auto is_test = decoded.alu == decoder::alu_operation::test;
    const auto is_byte = dst.width == decoder::operand_width::byte;
    if (is_cmp || is_test) {
        out << "  update_flags_cmp(" << (is_byte ? byte_reg_access(dst.reg) : word_reg_access(dst.reg)) << ",";
        if (decoded.operands[1].kind == decoder::operand_kind::immediate)
            out << "0x" << std::hex << decoded.operands[1].immediate << std::dec;
        else { const auto& s = decoded.operands[1]; out << (s.width == decoder::operand_width::byte ? byte_reg_access(s.reg) : word_reg_access(s.reg)); }
        out << "," << (is_byte ? "true" : "false") << ");\n";
        return;
    }
    static constexpr const char* ops[] = {"", "+=", "+=", "|=", "-=", "&=", "-=", "^=", "", ""};
    out << "  " << word_reg_access(dst.reg) << " " << ops[static_cast<int>(decoded.alu)] << " 0x" << std::hex << decoded.operands[1].immediate << std::dec << ";\n"
        << "  update_flags_cmp(" << word_reg_access(dst.reg) << ", 0, false);\n";
}

void emit_string_op(std::ostream& out, const loader::program_image& image, const decoder::instruction& decoded) {
    const auto op = std::to_integer<std::uint8_t>(image.bytes[decoded.offset + decoded.size - 1]);
    const bool word = (op & 1U) != 0;
    const int d = word ? 2 : 1;
    const int base = op & 0xfeU;
    const bool has_rep = decoded.rep != decoder::rep_prefix::none;
    const auto wrap = [&](const std::string& body) {
        if (!has_rep) { out << "  { " << body << " }\n"; return; }
        out << "  while (regs[1] > 0) { regs[1]--; " << body;
        if (decoded.rep == decoder::rep_prefix::repe) out << " if (!(flags & 0x40u)) break;";
        else if (decoded.rep == decoder::rep_prefix::repne) out << " if ((flags & 0x40u)) break;";
        out << " }\n";
    };
    const auto sd = std::to_string(d);
    const auto step_si = "regs[14] = (uint16_t)(regs[14] + df_step(" + sd + "));";
    const auto step_di = "regs[15] = (uint16_t)(regs[15] + df_step(" + sd + "));";
    if (base == 0xa4) {
        if (word) wrap("uint16_t __v = (uint16_t)mem[regs[14]] | ((uint16_t)mem[regs[14]+1] << 8); "
            "mem[regs[15]] = (uint8_t)__v; mem[regs[15]+1] = (uint8_t)(__v >> 8); " + step_si + " " + step_di);
        else wrap("mem[regs[15]] = mem[regs[14]]; " + step_si + " " + step_di);
    } else if (base == 0xaa) {
        if (word) wrap("mem[regs[15]] = (uint8_t)regs[0]; mem[regs[15]+1] = (uint8_t)(regs[0] >> 8); " + step_di);
        else wrap("mem[regs[15]] = (uint8_t)regs[0]; " + step_di);
    } else if (base == 0xac) {
        if (word) wrap("regs[0] = (uint16_t)mem[regs[14]] | ((uint16_t)mem[regs[14]+1] << 8); " + step_si);
        else wrap("regs[0] = (regs[0] & 0xff00) | mem[regs[14]]; " + step_si);
    } else if (base == 0xa6) {
        if (word) wrap("uint16_t __a = (uint16_t)mem[regs[14]] | ((uint16_t)mem[regs[14]+1] << 8); "
            "uint16_t __b = (uint16_t)mem[regs[15]] | ((uint16_t)mem[regs[15]+1] << 8); "
            "update_flags_cmp(__a, __b, false); " + step_si + " " + step_di);
        else wrap("update_flags_cmp(mem[regs[14]], mem[regs[15]], true); " + step_si + " " + step_di);
    } else if (base == 0xae) {
        if (word) wrap("uint16_t __b = (uint16_t)mem[regs[15]] | ((uint16_t)mem[regs[15]+1] << 8); "
            "update_flags_cmp(regs[0], __b, false); " + step_di);
        else wrap("update_flags_cmp((uint8_t)regs[0], mem[regs[15]], true); " + step_di);
    } else out << "  // unsupported string op 0x" << std::hex << op << std::dec << "\n";
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

std::expected<void, branching_compile_error> emit_block_body(std::ostream& out, const loader::program_image& image, std::size_t start, std::size_t stop_before, const std::set<std::size_t>& block_targets);

std::expected<void, branching_compile_error> emit_block_at(std::ostream& out, const loader::program_image& image, std::size_t start, const std::set<std::size_t>& block_targets) {
    out << "static inline __attribute__((always_inline)) void " << block_label(start) << "() {\n";
    std::size_t loop_end = 0; decoder::branch_condition loop_cond{};
    for (std::size_t scan = start; scan < image.bytes.size(); ) {
        const auto probe = decoder::instruction_decoder::decode_at(image.bytes, scan);
        if (!probe) break;
        const auto next = scan + probe->size;
        const bool term = probe->kind == decoder::instruction_kind::jump || probe->kind == decoder::instruction_kind::conditional_jump
            || probe->kind == decoder::instruction_kind::return_ || probe->kind == decoder::instruction_kind::interrupt
            || probe->kind == decoder::instruction_kind::call;
        if (term && probe->kind == decoder::instruction_kind::conditional_jump && scan != start
            && static_cast<std::size_t>(next + probe->relative_target) == start) { loop_end = scan; loop_cond = probe->condition; }
        if (term) break;
        scan = next;
    }
    if (loop_end != 0) {
        out << "  do {\n";
        const auto body_result = emit_block_body(out, image, start, loop_end, block_targets);
        if (!body_result) return body_result;
        out << "  } while (";
        if (!emit_conditional(out, loop_cond)) out << "false";
        out << ");\n";
    } else { const auto body_result = emit_block_body(out, image, start, 0, block_targets); if (!body_result) return body_result; }
    out << "}\n\n";
    return {};
}

std::expected<void, branching_compile_error>
emit_block_body(std::ostream& out, const loader::program_image& image, std::size_t start, std::size_t stop_before, const std::set<std::size_t>& block_targets) {
    std::size_t position = start;
    while (position < image.bytes.size()) {
        if (stop_before != 0 && position == stop_before) break;
        const auto decoded = decoder::instruction_decoder::decode_at(image.bytes, position);
        if (!decoded) return std::unexpected(branching_compile_error{std::string{"cannot decode at "} + std::to_string(position) + ": " + decoded.error().message});
        const auto next = position + decoded->size;
        if (decoded->kind == decoder::instruction_kind::interrupt) {
            if (decoded->interrupt_number == 0x21) emit_int21_runtime(out);
            else if (decoded->interrupt_number == 0x10) emit_int10_runtime(out);
            else if (decoded->interrupt_number == 0x16) emit_int16_runtime(out);
            else { out << "  syscall(SYS_exit, 1); return;\n"; position = next; break; }
            if (next < image.bytes.size() && block_targets.contains(next)) out << "  " << block_label(next) << "();\n";
            position = next; break;
        }
        if (decoded->kind == decoder::instruction_kind::jump) {
            if (decoded->indirect) {
                if (decoded->operands[0].kind == decoder::operand_kind::reg) out << "  dispatch(" << word_reg_access(decoded->operands[0].reg) << ");\n";
                else { out << "  // unsupported indirect jump\n  syscall(SYS_exit, 1); return;\n"; position = next; break; }
            } else out << "  " << block_label(position + decoded->size + decoded->relative_target) << "();\n";
            position = next; break;
        }
        if (decoded->kind == decoder::instruction_kind::conditional_jump) {
            out << "  if (";
            if (!emit_conditional(out, decoded->condition)) out << "/* unsupported cond */ false";
            out << ") " << block_label(position + decoded->size + decoded->relative_target)
                << "(); else " << block_label(next) << "();\n";
            position = next; break;
        }
        if (decoded->kind == decoder::instruction_kind::return_) {
            out << "  if (call_sp == 0) { syscall(SYS_exit, regs[0] & 0xff); return; }\n  dispatch(call_stack[--call_sp]); return;\n";
            position = next; break;
        }
        if (decoded->kind == decoder::instruction_kind::string) { emit_string_op(out, image, *decoded); position = next; break; }
        if (decoded->kind == decoder::instruction_kind::flags && position + 1 < image.bytes.size()) {
            const auto b = std::to_integer<std::uint8_t>(image.bytes[position + 1]);
            if (b == 0xfc) { out << "  flags &= ~0x400u;\n"; position = next; continue; }
            if (b == 0xfd) { out << "  flags |= 0x400u;\n"; position = next; continue; }
        }
        if (decoded->kind == decoder::instruction_kind::call) {
            if (decoded->indirect) { out << "  // indirect CALL\n  syscall(SYS_exit, 1); return;\n"; position = next; break; }
            out << "  call_stack[call_sp++] = " << (position + decoded->size) << "u;\n  "
                << block_label(position + decoded->size + decoded->relative_target) << "();\n";
            position = next; break;
        }
        switch (decoded->kind) {
        case decoder::instruction_kind::move_immediate: emit_move_immediate(out, *decoded); break;
        case decoder::instruction_kind::move: emit_move(out, *decoded); break;
        case decoder::instruction_kind::arithmetic:
        case decoder::instruction_kind::compare: emit_arithmetic(out, *decoded); break;
        default:
            out << "  // unsupported instruction at 0x" << std::hex << position << std::dec << "\n  syscall(SYS_exit, 1); return;\n";
            position = next; break;
        }
        position = next;
    }
    return {};
}

[[nodiscard]] std::expected<std::set<std::size_t>, branching_compile_error>
collect_block_starts(const loader::program_image& image) {
    std::set<std::size_t> starts;
    starts.insert(image.entry_offset());
    std::size_t last_interrupt = 0;
    for (std::size_t p = 0; p + 1 < image.bytes.size(); ++p)
        if (image.bytes[p] == std::byte{0xcd} && image.bytes[p + 1] == std::byte{0x21}) last_interrupt = p;
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto start : std::vector<std::size_t>(starts.begin(), starts.end())) {
            std::size_t position = start;
            while (position < image.bytes.size()) {
                const auto decoded = decoder::instruction_decoder::decode_at(image.bytes, position);
                if (!decoded) return std::unexpected(branching_compile_error{std::string{"cannot decode at "} + std::to_string(position) + ": " + decoded.error().message});
                const auto next = position + decoded->size;
                const auto kind = decoded->kind;
                const bool is_branch = kind == decoder::instruction_kind::jump || kind == decoder::instruction_kind::conditional_jump || kind == decoder::instruction_kind::call;
                if (is_branch && starts.insert(position + decoded->size + decoded->relative_target).second) changed = true;
                const bool falls_through = kind == decoder::instruction_kind::conditional_jump || kind == decoder::instruction_kind::call
                    || (kind == decoder::instruction_kind::interrupt && position != last_interrupt);
                if (falls_through && next < image.bytes.size() && starts.insert(next).second) changed = true;
                if (kind == decoder::instruction_kind::jump || kind == decoder::instruction_kind::conditional_jump
                    || kind == decoder::instruction_kind::return_ || kind == decoder::instruction_kind::interrupt
                    || kind == decoder::instruction_kind::call) break;
                position = next;
            }
        }
    }
    for (std::size_t pos = 0; pos < image.bytes.size() && starts.size() < 256; ) {
        const auto decoded = decoder::instruction_decoder::decode_at(image.bytes, pos); if (!decoded) break;
        starts.insert(pos);
        pos += decoded->size;
        if (decoded->kind == decoder::instruction_kind::interrupt) break;
    }
    return starts;
}

void emit_image_array(std::ostream& out, const std::vector<std::byte>& bytes) {
    out << "  static const uint8_t img[] = {";
    for (std::size_t i = 0; i < bytes.size(); ++i) { if (i % 16 == 0) out << "\n    ";
        out << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(std::to_integer<std::uint8_t>(bytes[i])) << std::dec << ","; }
    out << "\n  };\n";
}
[[nodiscard]] std::expected<void, branching_compile_error> write_main(std::ostream& out, const loader::program_image& image) {
    out << "int main() {\n  regs[4] = 0xfffe;\n";
    if (image.format == loader::executable_format::mz) {
        const auto base = static_cast<std::uint32_t>(image.entry_point.segment) * 16U;
        const auto seg = image.entry_point.segment;
        emit_image_array(out, image.bytes);
        out << "  for (size_t i = 0; i < sizeof(img); ++i) mem[" << std::hex << base << " + i] = img[i];\n" << std::dec;
        for (const auto& reloc : image.relocations) {
            const auto offset = static_cast<std::uint32_t>(reloc.address.segment) * 16U + reloc.address.offset + base;
            out << "  { uint16_t cur = (uint16_t)mem[" << std::hex << offset << "] | ((uint16_t)mem["
                << offset + 1 << "] << 8); cur = (uint16_t)(cur + " << seg << "); mem["
                << offset << "] = (uint8_t)cur; mem[" << offset + 1 << "] = (uint8_t)(cur >> 8); }\n" << std::dec;
        }
        out << "  regs[10] = " << image.initial_stack.segment << "; regs[4] = " << image.initial_stack.offset << ";\n";
    } else {
        emit_image_array(out, image.bytes);
        out << "  for (size_t i = 0; i < sizeof(img); ++i) mem[0x100 + i] = img[i];\n";
    }
    out << "  " << block_label(image.entry_offset()) << "();\n  return 0;\n}\n";
    return {};
}

[[nodiscard]] std::expected<void, branching_compile_error>
emit_dispatch(std::ostream& out, const std::set<std::size_t>& targets) {
    out << "static void dispatch(uint32_t target) {\n  switch (target) {\n";
    for (const auto t : targets)
        out << "    case 0x" << std::hex << t << std::dec << ": " << block_label(t) << "(); return;\n";
    out << "    default: syscall(SYS_exit, 1); return;\n  }\n}\n\n"; return {}; }

std::expected<std::string, branching_compile_error> emit_c_runtime(const loader::program_image& image) {
    const auto starts = collect_block_starts(image);
    if (!starts) return std::unexpected(starts.error());
    std::ostringstream out;
    emit_runtime_header(out);
    for (const auto start : *starts) out << "static inline __attribute__((always_inline)) void " << block_label(start) << "();\n";
    for (const auto start : *starts) { const auto r = emit_block_at(out, image, start, *starts); if (!r) return std::unexpected(r.error()); }
    const auto dr = emit_dispatch(out, *starts); if (!dr) return std::unexpected(dr.error());
    const auto mr = write_main(out, image); if (!mr) return std::unexpected(mr.error());
    return out.str();
}
} // namespace

std::expected<std::uint8_t, branching_compile_error> branching_compiler::extract_exit_code(const loader::program_image& image) {
    const auto result = straight_line_compiler::extract_exit_code(image);
    if (!result) return std::unexpected(branching_compile_error{result.error().message});
    return *result;
}
std::expected<std::vector<std::byte>, branching_compile_error> branching_compiler::compile(const loader::program_image& image) {
    const auto c_source = emit_c_runtime(image);
    if (!c_source) return std::unexpected(c_source.error());
    const auto tmp_dir = std::filesystem::temp_directory_path();
    const auto c_path = tmp_dir / "dosrecomp_branching.cpp";
    const auto out_path = tmp_dir / "dosrecomp_branching.elf";
    std::error_code ec;
    std::filesystem::remove(out_path, ec);
    { std::ofstream c_file(c_path, std::ios::binary | std::ios::trunc);
      if (!c_file) return std::unexpected(branching_compile_error{"cannot write " + c_path.string()});
      c_file.write(c_source->data(), static_cast<std::streamsize>(c_source->size())); }
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
std::expected<std::string, branching_compile_error> branching_compiler::emit_llvm(const loader::program_image& image) {
    const auto result = straight_line_compiler::emit_llvm(image);
    if (!result) return std::unexpected(branching_compile_error{result.error().message});
    return *result;
}

} // namespace dosrecomp::compiler