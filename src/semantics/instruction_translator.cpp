#include "dosrecomp/semantics/instruction_translator.hpp"

#include <optional>

namespace dosrecomp::semantics {
namespace {
[[nodiscard]] std::optional<ir::register_id> register_for(decoder::register_name reg) {
    switch (reg) {
    case decoder::register_name::ax: return ir::register_id::ax;
    case decoder::register_name::bx: return ir::register_id::bx;
    case decoder::register_name::cx: return ir::register_id::cx;
    case decoder::register_name::dx: return ir::register_id::dx;
    case decoder::register_name::si: return ir::register_id::si;
    case decoder::register_name::di: return ir::register_id::di;
    case decoder::register_name::bp: return ir::register_id::bp;
    case decoder::register_name::sp: return ir::register_id::sp;
    default: return std::nullopt;
    }
}
} // namespace

std::expected<semantic_effect, translation_error>
instruction_translator::translate(const std::vector<std::byte>& code, const decoder::instruction& instruction,
                                 ir::register_ssa_builder& ssa, ir::register_state& state) {
    if (instruction.offset > code.size() || code.size() - instruction.offset < instruction.size) {
        return std::unexpected(translation_error{"decoded instruction exceeds source bytes"});
    }
    if (instruction.kind != decoder::instruction_kind::move_immediate || instruction.operand_count != 2 ||
        instruction.operands[0].kind != decoder::operand_kind::reg ||
        instruction.operands[0].width != decoder::operand_width::word ||
        instruction.operands[1].kind != decoder::operand_kind::immediate ||
        instruction.operands[1].width != decoder::operand_width::word) {
        return std::unexpected(translation_error{"instruction semantics are not implemented"});
    }
    const auto destination = register_for(instruction.operands[0].reg);
    if (!destination) return std::unexpected(translation_error{"MOV destination is not an SSA word register"});
    const auto immediate = instruction.operands[1].immediate;
    return semantic_effect{*destination, ssa.define_constant(state, *destination, immediate), immediate};
}

} // namespace dosrecomp::semantics
