#include "dosrecomp/compiler/cpp_backend.hpp"

#include "dosrecomp/cfg/cfg_builder.hpp"
#include "dosrecomp/decoder/instruction_decoder.hpp"
#include "dosrecomp/runtime/real_mode_memory.hpp"
#include "dosrecomp/semantics/instruction_translator.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace dosrecomp::compiler {
namespace {
[[nodiscard]] std::string reg_name(decoder::register_name reg, decoder::operand_width width) {
    static const std::array<std::string_view, 20> names{
        "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
        "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
        "es", "cs", "ss", "ds"
    };
    std::ostringstream out;
    if (width == decoder::operand_width::byte) out << "u8_";
    else out << "u16_";
    out << names[static_cast<std::size_t>(reg)];
    return out.str();
}

[[nodiscard]] std::string alu_op(decoder::alu_operation alu) {
    switch (alu) {
    case decoder::alu_operation::add: return "+";
    case decoder::alu_operation::subtract: return "-";
    case decoder::alu_operation::bit_and: return "&";
    case decoder::alu_operation::bit_or: return "|";
    case decoder::alu_operation::bit_xor: return "^";
    case decoder::alu_operation::adc: return "+carry";
    case decoder::alu_operation::sbb: return "-borrow";
    default: return "?";
    }
}

struct emit_state {
    std::ostringstream out;
    ir::register_ssa_builder ssa;
    ir::register_state state;
    decoder::register_values registers{};
    runtime::real_mode_memory memory;
    std::uint16_t exit_code = 0;
    bool got_exit = false;
};

void emit_instruction(const loader::program_image& image, std::size_t offset,
                      const decoder::instruction& decoded, emit_state& es) {
    if (decoded.kind == decoder::instruction_kind::move_immediate && decoded.operand_count == 2 &&
        decoded.operands[0].kind == decoder::operand_kind::reg) {
        const auto dst = reg_name(decoded.operands[0].reg, decoded.operands[0].width);
        const auto imm = decoded.operands[1].immediate;
        if (decoded.operands[0].width == decoder::operand_width::byte) {
            es.out << "  " << dst << " = 0x" << std::hex << (imm & 0xffU) << std::dec << "; // MOV\n";
        } else {
            es.out << "  " << dst << " = 0x" << std::hex << imm << std::dec << "; // MOV\n";
        }
        return;
    }
    if (decoded.kind == decoder::instruction_kind::interrupt) {
        es.out << "  dos_int21(ax(), dx()); // INT 21h handler resolved by host\n";
        return;
    }
    if (decoded.kind == decoder::instruction_kind::return_) {
        es.out << "  return al(); // RET (fallback exit)\n";
        return;
    }
    if (decoded.kind == decoder::instruction_kind::arithmetic && decoded.operand_count == 2 &&
        decoded.operands[0].kind == decoder::operand_kind::reg &&
        decoded.operands[1].kind == decoder::operand_kind::immediate) {
        const auto dst = reg_name(decoded.operands[0].reg, decoded.operands[0].width);
        es.out << "  " << dst << " = " << dst << " " << alu_op(decoded.alu) << " 0x"
               << std::hex << decoded.operands[1].immediate << std::dec << ";\n";
        return;
    }
    if (decoded.kind == decoder::instruction_kind::compare && decoded.operand_count == 2 &&
        decoded.operands[0].kind == decoder::operand_kind::reg &&
        decoded.operands[1].kind == decoder::operand_kind::immediate) {
        const auto dst = reg_name(decoded.operands[0].reg, decoded.operands[0].width);
        es.out << "  flags = cmp_" << (decoded.operands[0].width == decoder::operand_width::byte ? "byte" : "word")
               << "(" << dst << ", 0x" << std::hex << decoded.operands[1].immediate << std::dec << ");\n";
        return;
    }
    es.out << "  // unsupported: opcode 0x" << std::hex
           << static_cast<unsigned>(std::to_integer<std::uint8_t>(image.bytes[offset])) << std::dec << "\n";
}
}

std::expected<std::string, cpp_emit_error>
cpp_backend::emit(const loader::program_image& image) {
    const auto graph = cfg::cfg_builder::build(image.bytes, image.entry_offset());
    if (!graph) return std::unexpected(cpp_emit_error{std::string{"cannot build CFG: "} + graph.error().message});
    if (graph->blocks.empty()) return std::unexpected(cpp_emit_error{"no reachable blocks"});
    emit_state es;
    es.state = es.ssa.entry_state();
    es.registers[static_cast<std::size_t>(decoder::register_name::sp)] = 0xfffe;
    es.out << "// Auto-generated C++ from DOS executable\n";
    es.out << "#include <cstdint>\n";
    es.out << "namespace dosrecomp_runtime {\n";
    es.out << "  inline std::uint16_t ax = 0;\n";
    es.out << "  inline std::uint16_t bx = 0;\n";
    es.out << "  inline std::uint16_t cx = 0;\n";
    es.out << "  inline std::uint16_t dx = 0;\n";
    es.out << "  inline std::uint16_t si = 0;\n";
    es.out << "  inline std::uint16_t di = 0;\n";
    es.out << "  inline std::uint16_t sp = 0;\n";
    es.out << "  inline std::uint16_t bp = 0;\n";
    es.out << "  inline std::uint32_t flags = 0;\n";
    es.out << "  inline std::uint8_t al() { return static_cast<std::uint8_t>(ax); }\n";
    es.out << "  inline std::uint8_t ah() { return static_cast<std::uint8_t>(ax >> 8); }\n";
    es.out << "  inline void set_al(std::uint8_t v) { ax = static_cast<std::uint16_t>((ax & 0xff00) | v); }\n";
    es.out << "  inline void set_ah(std::uint8_t v) { ax = static_cast<std::uint16_t>((ax & 0x00ff) | (v << 8)); }\n";
    es.out << "  inline std::uint32_t cmp_byte(std::uint16_t a, std::uint8_t b) {\n";
    es.out << "    const std::uint16_t r = static_cast<std::uint16_t>(static_cast<std::uint8_t>(a) - b);\n";
    es.out << "    return (r == 0 ? 0x40u : 0u) | ((r & 0x80) ? 0x80u : 0u) | (static_cast<std::uint8_t>(a) < b ? 1u : 0u);\n";
    es.out << "  }\n";
    es.out << "  inline std::uint32_t cmp_word(std::uint16_t a, std::uint16_t b) {\n";
    es.out << "    const std::uint16_t r = static_cast<std::uint16_t>(a - b);\n";
    es.out << "    return (r == 0 ? 0x40u : 0u) | ((r & 0x8000) ? 0x80u : 0u) | (a < b ? 1u : 0u);\n";
    es.out << "  }\n";
    es.out << "  inline int dos_int21(int /*ax*/, int /*dx*/) { return 0; }\n";
    es.out << "}\n";
    es.out << "int main() {\n";
    es.out << "  using namespace dosrecomp_runtime;\n";
    std::size_t position = graph->blocks.front().start;
    while (position < image.bytes.size()) {
        const auto decoded = decoder::instruction_decoder::decode_at(image.bytes, position);
        if (!decoded) return std::unexpected(cpp_emit_error{std::string{"cannot decode at "} + std::to_string(position) + ": " + decoded.error().message});
        emit_instruction(image, position, *decoded, es);
        position += decoded->size;
        if (decoded->kind == decoder::instruction_kind::interrupt) break;
        if (decoded->kind == decoder::instruction_kind::return_) break;
    }
    es.out << "  return al();\n";
    es.out << "}\n";
    return es.out.str();
}

} // namespace dosrecomp::compiler