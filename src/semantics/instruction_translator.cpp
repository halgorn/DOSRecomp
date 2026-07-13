#include "dosrecomp/semantics/instruction_translator.hpp"

#include "dosrecomp/runtime/real_mode_memory.hpp"

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
    case decoder::alu_operation::test: return ir::operation_kind::test;
    default: return std::nullopt;
    }
}
[[nodiscard]] std::optional<ir::register_id> register_for(decoder::register_name reg) {
    switch (reg) {
    case decoder::register_name::al: return ir::register_id::al;
    case decoder::register_name::cl: return ir::register_id::cl;
    case decoder::register_name::dl: return ir::register_id::dl;
    case decoder::register_name::bl: return ir::register_id::bl;
    case decoder::register_name::ah: return ir::register_id::ah;
    case decoder::register_name::ch: return ir::register_id::ch;
    case decoder::register_name::dh: return ir::register_id::dh;
    case decoder::register_name::bh: return ir::register_id::bh;
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
[[nodiscard]] std::uint16_t concrete_value(ir::register_id reg, const decoder::register_values& values) {
    switch (reg) {
    case ir::register_id::al: return static_cast<std::uint16_t>(values[static_cast<std::size_t>(decoder::register_name::al)]);
    case ir::register_id::cl: return static_cast<std::uint16_t>(values[static_cast<std::size_t>(decoder::register_name::cl)]);
    case ir::register_id::dl: return static_cast<std::uint16_t>(values[static_cast<std::size_t>(decoder::register_name::dl)]);
    case ir::register_id::bl: return static_cast<std::uint16_t>(values[static_cast<std::size_t>(decoder::register_name::bl)]);
    case ir::register_id::ah: return static_cast<std::uint16_t>(values[static_cast<std::size_t>(decoder::register_name::ah)]);
    case ir::register_id::ch: return static_cast<std::uint16_t>(values[static_cast<std::size_t>(decoder::register_name::ch)]);
    case ir::register_id::dh: return static_cast<std::uint16_t>(values[static_cast<std::size_t>(decoder::register_name::dh)]);
    case ir::register_id::bh: return static_cast<std::uint16_t>(values[static_cast<std::size_t>(decoder::register_name::bh)]);
    case ir::register_id::ax: return values[static_cast<std::size_t>(decoder::register_name::ax)];
    case ir::register_id::bx: return values[static_cast<std::size_t>(decoder::register_name::bx)];
    case ir::register_id::cx: return values[static_cast<std::size_t>(decoder::register_name::cx)];
    case ir::register_id::dx: return values[static_cast<std::size_t>(decoder::register_name::dx)];
    case ir::register_id::si: return values[static_cast<std::size_t>(decoder::register_name::si)];
    case ir::register_id::di: return values[static_cast<std::size_t>(decoder::register_name::di)];
    case ir::register_id::bp: return values[static_cast<std::size_t>(decoder::register_name::bp)];
    case ir::register_id::sp: return values[static_cast<std::size_t>(decoder::register_name::sp)];
    default: return 0;
    }
}
void store_concrete(ir::register_id reg, std::uint16_t value, decoder::register_values& values) {
    switch (reg) {
    case ir::register_id::al: values[static_cast<std::size_t>(decoder::register_name::al)] = static_cast<std::uint8_t>(value); break;
    case ir::register_id::cl: values[static_cast<std::size_t>(decoder::register_name::cl)] = static_cast<std::uint8_t>(value); break;
    case ir::register_id::dl: values[static_cast<std::size_t>(decoder::register_name::dl)] = static_cast<std::uint8_t>(value); break;
    case ir::register_id::bl: values[static_cast<std::size_t>(decoder::register_name::bl)] = static_cast<std::uint8_t>(value); break;
    case ir::register_id::ah: values[static_cast<std::size_t>(decoder::register_name::ah)] = static_cast<std::uint8_t>(value); break;
    case ir::register_id::ch: values[static_cast<std::size_t>(decoder::register_name::ch)] = static_cast<std::uint8_t>(value); break;
    case ir::register_id::dh: values[static_cast<std::size_t>(decoder::register_name::dh)] = static_cast<std::uint8_t>(value); break;
    case ir::register_id::bh: values[static_cast<std::size_t>(decoder::register_name::bh)] = static_cast<std::uint8_t>(value); break;
    case ir::register_id::ax: values[static_cast<std::size_t>(decoder::register_name::ax)] = value; break;
    case ir::register_id::bx: values[static_cast<std::size_t>(decoder::register_name::bx)] = value; break;
    case ir::register_id::cx: values[static_cast<std::size_t>(decoder::register_name::cx)] = value; break;
    case ir::register_id::dx: values[static_cast<std::size_t>(decoder::register_name::dx)] = value; break;
    case ir::register_id::si: values[static_cast<std::size_t>(decoder::register_name::si)] = value; break;
    case ir::register_id::di: values[static_cast<std::size_t>(decoder::register_name::di)] = value; break;
    case ir::register_id::bp: values[static_cast<std::size_t>(decoder::register_name::bp)] = value; break;
    case ir::register_id::sp: values[static_cast<std::size_t>(decoder::register_name::sp)] = value; break;
    default: break;
    }
}
} // namespace

std::expected<semantic_effect, translation_error>
instruction_translator::translate(const std::vector<std::byte>& code, const decoder::instruction& instruction,
                                 ir::register_ssa_builder& ssa, ir::register_state& state,
                                 decoder::register_values* registers, runtime::real_mode_memory* memory,
                                 std::uint16_t default_segment) {
    if (instruction.offset > code.size() || code.size() - instruction.offset < instruction.size) {
        return std::unexpected(translation_error{"decoded instruction exceeds source bytes"});
    }
    if ((instruction.kind == decoder::instruction_kind::arithmetic || instruction.kind == decoder::instruction_kind::compare) && instruction.operand_count == 2 &&
        instruction.operands[0].kind == decoder::operand_kind::reg && instruction.operands[0].width == decoder::operand_width::word &&
        (instruction.operands[1].kind == decoder::operand_kind::immediate || instruction.operands[1].kind == decoder::operand_kind::reg) &&
        instruction.operands[1].width == decoder::operand_width::word) {
        const auto destination = register_for(instruction.operands[0].reg);
        const auto operation = operation_for(instruction.alu);
        if (!destination || !operation) return std::unexpected(translation_error{"arithmetic instruction semantics are not implemented"});
        const auto input = state.values[static_cast<std::size_t>(*destination)];
        std::size_t source_value;
        if (instruction.operands[1].kind == decoder::operand_kind::immediate) {
            source_value = ssa.constant(*destination, instruction.operands[1].immediate);
        } else {
            const auto source = register_for(instruction.operands[1].reg);
            if (!source) return std::unexpected(translation_error{"arithmetic source register is not an SSA word register"});
            source_value = state.values[static_cast<std::size_t>(*source)];
        }
        const auto result_register = (*operation == ir::operation_kind::compare || *operation == ir::operation_kind::test)
            ? ir::register_id::flags : *destination;
        return semantic_effect{result_register, ssa.define_operation(state, result_register, *operation, {input, source_value}), std::nullopt, std::nullopt};
    }
    if (instruction.kind == decoder::instruction_kind::call && instruction.size == 3) {
        if (!registers || !memory) return std::unexpected(translation_error{"CALL requires concrete register and memory state"});
        const auto current_sp = static_cast<std::uint16_t>(concrete_value(ir::register_id::sp, *registers));
        const auto new_sp = static_cast<std::uint16_t>(current_sp - 2U);
        const auto return_ip = static_cast<std::uint16_t>(instruction.offset + instruction.size);
        const auto written = memory->write16(default_segment, new_sp, return_ip);
        if (!written) return std::unexpected(translation_error{std::string{"cannot write CALL return address: "} + written.error().message});
        store_concrete(ir::register_id::sp, new_sp, *registers);
        const auto physical = static_cast<std::uint32_t>(default_segment) * 16U + new_sp;
        return semantic_effect{ir::register_id::sp, ssa.define_constant(state, ir::register_id::sp, new_sp), new_sp,
            memory_access{true, physical, decoder::operand_width::word}};
    }
    if (instruction.kind == decoder::instruction_kind::return_ && (instruction.size == 1 || instruction.size == 3)) {
        if (!registers || !memory) return std::unexpected(translation_error{"RET requires concrete register and memory state"});
        const auto current_sp = static_cast<std::uint16_t>(concrete_value(ir::register_id::sp, *registers));
        const auto read = memory->read16(default_segment, current_sp);
        if (!read) return std::unexpected(translation_error{std::string{"cannot read RET address: "} + read.error().message});
        std::uint16_t adjust = 2;
        if (instruction.size == 3) {
            const auto raw = static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(code[instruction.offset + 1])) |
                             static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(code[instruction.offset + 2])) << 8U;
            adjust = static_cast<std::uint16_t>(adjust + raw);
        }
        const auto new_sp = static_cast<std::uint16_t>(current_sp + adjust);
        store_concrete(ir::register_id::sp, new_sp, *registers);
        const auto physical = static_cast<std::uint32_t>(default_segment) * 16U + current_sp;
        return semantic_effect{ir::register_id::sp, ssa.define_constant(state, ir::register_id::sp, new_sp), new_sp,
            memory_access{false, physical, decoder::operand_width::word}};
    }
    if (instruction.kind == decoder::instruction_kind::stack && instruction.size == 1 && instruction.operand_count == 1 &&
        instruction.operands[0].kind == decoder::operand_kind::reg && instruction.operands[0].width == decoder::operand_width::word) {
        if (!registers || !memory) return std::unexpected(translation_error{"PUSH/POP requires concrete register and memory state"});
        const auto reg = register_for(instruction.operands[0].reg);
        if (!reg) return std::unexpected(translation_error{"PUSH/POP register is not an SSA word register"});
        const auto current_sp = static_cast<std::uint16_t>(concrete_value(ir::register_id::sp, *registers));
        if (instruction.stack_pop) {
            const auto read = memory->read16(default_segment, current_sp);
            if (!read) return std::unexpected(translation_error{std::string{"cannot read POP source: "} + read.error().message});
            const auto new_sp = static_cast<std::uint16_t>(current_sp + 2U);
            store_concrete(*reg, *read, *registers);
            store_concrete(ir::register_id::sp, new_sp, *registers);
            const auto physical = static_cast<std::uint32_t>(default_segment) * 16U + current_sp;
            return semantic_effect{*reg, ssa.define_constant(state, *reg, *read), *read,
                memory_access{false, physical, decoder::operand_width::word}};
        }
        const auto reg_value = concrete_value(*reg, *registers);
        const auto new_sp = static_cast<std::uint16_t>(current_sp - 2U);
        const auto written = memory->write16(default_segment, new_sp, reg_value);
        if (!written) return std::unexpected(translation_error{std::string{"cannot write PUSH destination: "} + written.error().message});
        store_concrete(ir::register_id::sp, new_sp, *registers);
        const auto physical = static_cast<std::uint32_t>(default_segment) * 16U + new_sp;
        return semantic_effect{ir::register_id::sp, ssa.define_constant(state, ir::register_id::sp, new_sp), new_sp,
            memory_access{true, physical, decoder::operand_width::word}};
    }
    if ((instruction.kind != decoder::instruction_kind::move_immediate && instruction.kind != decoder::instruction_kind::move) || instruction.operand_count != 2) {
        return std::unexpected(translation_error{"instruction semantics are not implemented"});
    }
    const auto& destination_operand = instruction.operands[0];
    const auto& source_operand = instruction.operands[1];
    const auto op_width = destination_operand.width;
    const auto is_byte = op_width == decoder::operand_width::byte;
    if (destination_operand.kind == decoder::operand_kind::memory) {
        if (!registers || !memory) return std::unexpected(translation_error{"MOV memory write requires concrete register and memory state"});
        if (source_operand.kind != decoder::operand_kind::reg && source_operand.kind != decoder::operand_kind::immediate) {
            return std::unexpected(translation_error{"MOV memory write source must be a register or immediate"});
        }
        const auto offset = decoder::effective_address_resolver::resolve(destination_operand, *registers);
        if (!offset) return std::unexpected(translation_error{"cannot resolve MOV memory destination address"});
        const auto physical = static_cast<std::uint32_t>(default_segment) * 16U + *offset;
        std::uint16_t value = 0;
        if (source_operand.kind == decoder::operand_kind::immediate) value = source_operand.immediate;
        else {
            const auto source = register_for(source_operand.reg);
            if (!source) return std::unexpected(translation_error{"MOV memory source is not an SSA register"});
            value = concrete_value(*source, *registers);
        }
        std::expected<void, runtime::memory_error> written;
        if (is_byte) written = memory->write8(default_segment, *offset, static_cast<std::uint8_t>(value));
        else written = memory->write16(default_segment, *offset, value);
        if (!written) return std::unexpected(translation_error{std::string{"cannot write MOV memory destination: "} + written.error().message});
        return semantic_effect{ir::register_id::count, ssa.define(state, ir::register_id::count, {}), std::nullopt,
            memory_access{true, physical, op_width}};
    }
    if (destination_operand.kind != decoder::operand_kind::reg) {
        return std::unexpected(translation_error{"MOV destination must be a register or memory"});
    }
    const auto destination = register_for(destination_operand.reg);
    if (!destination) return std::unexpected(translation_error{"MOV destination is not an SSA register"});
    if (source_operand.kind == decoder::operand_kind::memory) {
        if (!registers || !memory) return std::unexpected(translation_error{"MOV memory read requires concrete register and memory state"});
        const auto offset = decoder::effective_address_resolver::resolve(source_operand, *registers);
        if (!offset) return std::unexpected(translation_error{"cannot resolve MOV memory source address"});
        const auto physical = static_cast<std::uint32_t>(default_segment) * 16U + *offset;
        if (is_byte) {
            const auto value = memory->read8(default_segment, *offset);
            if (!value) return std::unexpected(translation_error{std::string{"cannot read MOV memory source: "} + value.error().message});
            return semantic_effect{*destination, ssa.define_constant(state, *destination, *value), *value,
                memory_access{false, physical, op_width}};
        }
        const auto value = memory->read16(default_segment, *offset);
        if (!value) return std::unexpected(translation_error{std::string{"cannot read MOV memory source: "} + value.error().message});
        return semantic_effect{*destination, ssa.define_constant(state, *destination, *value), *value,
            memory_access{false, physical, op_width}};
    }
    if (source_operand.kind == decoder::operand_kind::immediate) {
        const auto immediate = source_operand.immediate;
        return semantic_effect{*destination, ssa.define_constant(state, *destination, immediate), immediate, std::nullopt};
    }
    const auto source = register_for(source_operand.reg);
    if (!source) return std::unexpected(translation_error{"MOV source is not an SSA register"});
    const auto source_value = state.values[static_cast<std::size_t>(*source)];
    return semantic_effect{*destination, ssa.define(state, *destination, {source_value}), std::nullopt, std::nullopt};
}

} // namespace dosrecomp::semantics