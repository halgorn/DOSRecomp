#include "dosrecomp/semantics/instruction_translator.hpp"

#include <optional>

namespace dosrecomp::semantics {
namespace {
[[nodiscard]] std::optional<ir::operation_kind> operation_for(decoder::alu_operation operation) {
    switch (operation) {
    case decoder::alu_operation::add: return ir::operation_kind::add;
    case decoder::alu_operation::subtract: return ir::operation_kind::subtract;
    case decoder::alu_operation::bit_and: return ir::operation_kind::bit_and;
    case decoder::alu_operation::bit_or: return ir::operation_kind::bit_or;
    case decoder::alu_operation::bit_xor: return ir::operation_kind::bit_xor;
    case decoder::alu_operation::compare: return ir::operation_kind::compare;
    default: return std::nullopt;
    }
}
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
    if ((instruction.kind == decoder::instruction_kind::arithmetic || instruction.kind == decoder::instruction_kind::compare) && instruction.operand_count == 2 &&
        instruction.operands[0].kind == decoder::operand_kind::reg && instruction.operands[0].width == decoder::operand_width::word &&
        instruction.operands[1].kind == decoder::operand_kind::immediate && instruction.operands[1].width == decoder::operand_width::word) {
        const auto destination = register_for(instruction.operands[0].reg);
        const auto operation = operation_for(instruction.alu);
        if (!destination || !operation) return std::unexpected(translation_error{"arithmetic instruction semantics are not implemented"});
        const auto input = state.values[static_cast<std::size_t>(*destination)];
        const auto immediate = ssa.constant(*destination, instruction.operands[1].immediate);
        const auto result_register = *operation == ir::operation_kind::compare ? ir::register_id::flags : *destination;
        return semantic_effect{result_register, ssa.define_operation(state, result_register, *operation, {input, immediate}), std::nullopt};
    }
    if ((instruction.kind != decoder::instruction_kind::move_immediate && instruction.kind != decoder::instruction_kind::move) || instruction.operand_count != 2 ||
        instruction.operands[0].kind != decoder::operand_kind::reg ||
        instruction.operands[0].width != decoder::operand_width::word ||
        instruction.operands[1].width != decoder::operand_width::word) {
        return std::unexpected(translation_error{"instruction semantics are not implemented"});
    }
    const auto destination = register_for(instruction.operands[0].reg);
    if (!destination) return std::unexpected(translation_error{"MOV destination is not an SSA word register"});
    if (instruction.operands[1].kind == decoder::operand_kind::immediate) {
        const auto immediate = instruction.operands[1].immediate;
        return semantic_effect{*destination, ssa.define_constant(state, *destination, immediate), immediate};
    }
    if (instruction.operands[1].kind != decoder::operand_kind::reg) {
        return std::unexpected(translation_error{"MOV memory semantics are not implemented"});
    }
    const auto source = register_for(instruction.operands[1].reg);
    if (!source) return std::unexpected(translation_error{"MOV source is not an SSA word register"});
    const auto source_value = state.values[static_cast<std::size_t>(*source)];
    return semantic_effect{*destination, ssa.define(state, *destination, {source_value}), std::nullopt};
}

} // namespace dosrecomp::semantics
