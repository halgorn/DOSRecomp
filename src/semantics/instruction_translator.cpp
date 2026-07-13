#include "dosrecomp/semantics/instruction_translator.hpp"

namespace dosrecomp::semantics {
namespace {
[[nodiscard]] std::uint8_t byte_at(const std::vector<std::byte>& code, std::size_t offset) {
    return std::to_integer<std::uint8_t>(code[offset]);
}

[[nodiscard]] ir::register_id register_for(std::uint8_t opcode) {
    constexpr ir::register_id mapping[] = {
        ir::register_id::ax, ir::register_id::cx, ir::register_id::dx, ir::register_id::bx,
        ir::register_id::sp, ir::register_id::bp, ir::register_id::si, ir::register_id::di,
    };
    return mapping[opcode - 0xb8U];
}
} // namespace

std::expected<semantic_effect, translation_error>
instruction_translator::translate(const std::vector<std::byte>& code, const decoder::instruction& instruction,
                                 ir::register_ssa_builder& ssa, ir::register_state& state) {
    if (instruction.offset > code.size() || code.size() - instruction.offset < instruction.size) {
        return std::unexpected(translation_error{"decoded instruction exceeds source bytes"});
    }
    const auto opcode = byte_at(code, instruction.offset);
    if (instruction.kind != decoder::instruction_kind::move_immediate || opcode < 0xb8 || opcode > 0xbf || instruction.size != 3) {
        return std::unexpected(translation_error{"instruction semantics are not implemented"});
    }
    const auto destination = register_for(opcode);
    const auto immediate = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(byte_at(code, instruction.offset + 1)) |
        (static_cast<std::uint16_t>(byte_at(code, instruction.offset + 2)) << 8U));
    return semantic_effect{destination, ssa.define_constant(state, destination, immediate), immediate};
}

} // namespace dosrecomp::semantics
