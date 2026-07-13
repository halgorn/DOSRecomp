#include "dosrecomp/compiler/straight_line_compiler.hpp"

#include "dosrecomp/backend/elf_writer.hpp"
#include "dosrecomp/cfg/cfg_builder.hpp"
#include "dosrecomp/decoder/instruction_decoder.hpp"
#include "dosrecomp/ir/control_flow_ir.hpp"
#include "dosrecomp/ir/register_ssa.hpp"
#include "dosrecomp/runtime/real_mode_memory.hpp"
#include "dosrecomp/semantics/instruction_translator.hpp"

#include <vector>

namespace dosrecomp::compiler {
namespace {
[[nodiscard]] std::uint16_t resolve_exit_code(ir::register_ssa_builder& ssa, const ir::register_state&,
                                             std::size_t ssa_value) {
    while (true) {
        const auto& value = ssa.values()[ssa_value];
        if (value.constant) return *value.constant;
        if (value.operation && value.inputs.size() == 2) {
            const auto& first = ssa.values()[value.inputs[0]];
            const auto& second = ssa.values()[value.inputs[1]];
            if (first.constant && second.constant) {
                switch (*value.operation) {
                case ir::operation_kind::add: return static_cast<std::uint16_t>(*first.constant + *second.constant);
                case ir::operation_kind::subtract: return static_cast<std::uint16_t>(*first.constant - *second.constant);
                case ir::operation_kind::bit_and: return static_cast<std::uint16_t>(*first.constant & *second.constant);
                case ir::operation_kind::bit_or: return static_cast<std::uint16_t>(*first.constant | *second.constant);
                case ir::operation_kind::bit_xor: return static_cast<std::uint16_t>(*first.constant ^ *second.constant);
                default: break;
                }
            }
        }
        if (value.inputs.empty()) return 0;
        ssa_value = value.inputs.front();
    }
}

[[nodiscard]] bool evaluate_condition(decoder::branch_condition condition, ir::register_ssa_builder& ssa, std::size_t flags_ssa) {
    const auto flags = ssa.resolve_compare_flags(flags_ssa);
    switch (condition) {
    case decoder::branch_condition::equal: return flags.zero;
    case decoder::branch_condition::not_equal: return !flags.zero;
    case decoder::branch_condition::below: return flags.carry;
    case decoder::branch_condition::above_or_equal: return !flags.carry;
    case decoder::branch_condition::below_or_equal: return flags.carry || flags.zero;
    case decoder::branch_condition::above: return !flags.carry && !flags.zero;
    case decoder::branch_condition::sign: return flags.sign;
    case decoder::branch_condition::not_sign: return !flags.sign;
    case decoder::branch_condition::overflow: return flags.overflow;
    case decoder::branch_condition::not_overflow: return !flags.overflow;
    case decoder::branch_condition::less: return flags.sign != flags.overflow;
    case decoder::branch_condition::greater_or_equal: return flags.sign == flags.overflow;
    case decoder::branch_condition::less_or_equal: return flags.zero || flags.sign != flags.overflow;
    case decoder::branch_condition::greater: return !flags.zero && flags.sign == flags.overflow;
    default: return false;
    }
}

struct translation_state {
    ir::register_ssa_builder ssa;
    ir::register_state state;
    decoder::register_values registers{};
    runtime::real_mode_memory memory;
    std::vector<std::vector<std::byte>> write_payloads;
    std::vector<backend::write_call> writes;
    std::vector<bool> visited_block;
};

[[nodiscard]] std::expected<std::uint8_t, straight_line_compile_error>
translate_block(const loader::program_image& image, const cfg::control_flow_graph& graph,
                std::size_t block_index, translation_state& ts) {
    if (block_index >= graph.blocks.size()) return std::unexpected(straight_line_compile_error{"control flow ends without INT 21h exit"});
    if (ts.visited_block[block_index]) return std::unexpected(straight_line_compile_error{"straight-line compiler does not support loops"});
    ts.visited_block[block_index] = true;
    std::size_t position = graph.blocks[block_index].start;
    while (true) {
        if (position >= image.bytes.size()) return std::unexpected(straight_line_compile_error{"control flow ends without INT 21h exit"});
        const auto decoded = decoder::instruction_decoder::decode_at(image.bytes, position);
        if (!decoded) return std::unexpected(straight_line_compile_error{"cannot decode instruction at offset " + std::to_string(position) + ": " + decoded.error().message});
        if (decoded->kind == decoder::instruction_kind::interrupt) {
            if (decoded->interrupt_number != 0x21 && decoded->interrupt_number != 0x10U) {
                return std::unexpected(straight_line_compile_error{"unsupported interrupt " + std::to_string(decoded->interrupt_number) + "h"});
            }
            const auto ah_ssa = ts.state.values[static_cast<std::size_t>(ir::register_id::ah)];
            const auto ah_constant = resolve_exit_code(ts.ssa, ts.state, ah_ssa);
            const auto ax_ssa = ts.state.values[static_cast<std::size_t>(ir::register_id::ax)];
            const auto ax_constant = resolve_exit_code(ts.ssa, ts.state, ax_ssa);
            const auto ah_value = ah_constant != 0 || ah_ssa == ax_ssa ? static_cast<std::uint8_t>(ah_constant) : static_cast<std::uint8_t>(ax_constant >> 8U);
            if (ah_value == 0x4cU) {
                const auto al_ssa = ts.state.values[static_cast<std::size_t>(ir::register_id::al)];
                const auto al_constant = resolve_exit_code(ts.ssa, ts.state, al_ssa);
                if (al_constant != 0 || al_ssa == ax_ssa) return static_cast<std::uint8_t>(al_constant);
                return static_cast<std::uint8_t>(ax_constant & 0xffU);
            }
            if (decoded->interrupt_number == 0x10U) {
                if (ah_value == 0x0eU) {
                    const auto al_ssa = ts.state.values[static_cast<std::size_t>(ir::register_id::al)];
                    const auto al_constant = resolve_exit_code(ts.ssa, ts.state, al_ssa);
                    ts.write_payloads.push_back({{static_cast<std::byte>(al_constant & 0xffU)}});
                    ts.writes.push_back(backend::write_call{std::span<const std::byte>{ts.write_payloads.back().data(), 1}, 1});
                    position += decoded->size;
                    continue;
                }
                if (ah_value == 0x02U) {
                    position += decoded->size;
                    continue;
                }
                return std::unexpected(straight_line_compile_error{"unsupported INT 10h function AH=" + std::to_string(static_cast<unsigned>(ah_value))});
            }
            if (ah_value == 0x09U) {
                const auto dx_ssa = ts.state.values[static_cast<std::size_t>(ir::register_id::dx)];
                const auto dx_value = resolve_exit_code(ts.ssa, ts.state, dx_ssa);
                std::vector<std::byte> payload;
                std::size_t cursor = dx_value;
                while (cursor + 1 <= ts.memory.size) {
                    const auto value = ts.memory.read8(cursor);
                    if (!value) return std::unexpected(straight_line_compile_error{"cannot read INT 21h/AH=09h string"});
                    if (*value == '$') break;
                    payload.push_back(static_cast<std::byte>(*value));
                    ++cursor;
                }
                if (cursor + 1 > ts.memory.size) return std::unexpected(straight_line_compile_error{"INT 21h/AH=09h string is missing '$' terminator"});
                ts.write_payloads.push_back(std::move(payload));
                ts.writes.push_back(backend::write_call{std::span<const std::byte>{ts.write_payloads.back().data(), ts.write_payloads.back().size()}, 1});
                position += decoded->size;
                continue;
            }
            if (ah_value == 0x02U) {
                const auto dl_ssa = ts.state.values[static_cast<std::size_t>(ir::register_id::dl)];
                const auto dl_constant = resolve_exit_code(ts.ssa, ts.state, dl_ssa);
                ts.write_payloads.push_back({{static_cast<std::byte>(dl_constant & 0xffU)}});
                ts.writes.push_back(backend::write_call{std::span<const std::byte>{ts.write_payloads.back().data(), 1}, 2});
                position += decoded->size;
                continue;
            }
            return std::unexpected(straight_line_compile_error{"unsupported INT 21h function AH=" + std::to_string(static_cast<unsigned>(ah_value))});
        }
        if (decoded->kind == decoder::instruction_kind::return_) {
            return std::unexpected(straight_line_compile_error{"control flow ends without INT 21h exit"});
        }
        if (decoded->kind == decoder::instruction_kind::jump) {
            const auto target = static_cast<std::size_t>(position + decoded->size + decoded->relative_target);
            auto found = graph.blocks.size();
            for (std::size_t index = 0; index < graph.blocks.size(); ++index) {
                if (graph.blocks[index].start == target) { found = index; break; }
            }
            if (found == graph.blocks.size()) return std::unexpected(straight_line_compile_error{"straight-line compiler cannot resolve unconditional jump target"});
            return translate_block(image, graph, found, ts);
        }
        if (decoded->kind == decoder::instruction_kind::conditional_jump || decoded->kind == decoder::instruction_kind::loop) {
            const auto target = static_cast<std::size_t>(position + decoded->size + decoded->relative_target);
            auto target_block = graph.blocks.size();
            auto fallthrough_block = graph.blocks.size();
            for (std::size_t index = 0; index < graph.blocks.size(); ++index) {
                if (graph.blocks[index].start == target) target_block = index;
                if (graph.blocks[index].start == position + decoded->size) fallthrough_block = index;
            }
            if (target_block == graph.blocks.size() || fallthrough_block == graph.blocks.size()) {
                return std::unexpected(straight_line_compile_error{"straight-line compiler cannot resolve conditional branch targets"});
            }
            const auto flags_ssa = ts.state.values[static_cast<std::size_t>(ir::register_id::flags)];
            const auto taken = evaluate_condition(decoded->condition, ts.ssa, flags_ssa);
            return translate_block(image, graph, taken ? target_block : fallthrough_block, ts);
        }
        const auto effect = semantics::instruction_translator::translate(image.bytes, *decoded, ts.ssa, ts.state, &ts.registers, &ts.memory);
        if (!effect) return std::unexpected(straight_line_compile_error{std::string{"cannot translate instruction at offset "} + std::to_string(position) + ": " + effect.error().message});
        position += decoded->size;
    }
}
} // namespace

std::expected<std::uint8_t, straight_line_compile_error>
straight_line_compiler::extract_exit_code(const loader::program_image& image) {
    const auto graph = cfg::cfg_builder::build(image.bytes, image.entry_offset());
    if (!graph) return std::unexpected(straight_line_compile_error{std::string{"cannot build CFG: "} + graph.error().message});
    const auto ir = ir::control_flow_lowerer::lower(*graph);
    if (!ir) return std::unexpected(straight_line_compile_error{std::string{"cannot lower IR: "} + ir.error().message});
    if (graph->blocks.empty()) return std::unexpected(straight_line_compile_error{"straight-line compiler found no reachable blocks"});
    translation_state ts;
    ts.state = ts.ssa.entry_state();
    ts.visited_block.assign(graph->blocks.size(), false);
    ts.registers[static_cast<std::size_t>(decoder::register_name::sp)] = 0xfffe;
    if (image.format == loader::executable_format::mz) {
        ts.registers[static_cast<std::size_t>(decoder::register_name::ss)] = image.initial_stack.segment;
        ts.registers[static_cast<std::size_t>(decoder::register_name::sp)] = image.initial_stack.offset;
    }
    if (image.format == loader::executable_format::mz) {
        const auto first_byte = static_cast<std::uint32_t>(image.entry_point.segment) * 16U;
        const auto loaded = ts.memory.write(first_byte, std::span<const std::byte>{image.bytes.data(), image.bytes.size()});
        if (!loaded) return std::unexpected(straight_line_compile_error{std::string{"cannot load MZ image into real-mode memory: "} + loaded.error().message});
    } else {
        const auto loaded = ts.memory.write(0x100, std::span<const std::byte>{image.bytes.data(), image.bytes.size()});
        if (!loaded) return std::unexpected(straight_line_compile_error{std::string{"cannot load COM image into real-mode memory: "} + loaded.error().message});
    }
    return translate_block(image, *graph, 0, ts);
}

std::expected<std::vector<std::byte>, straight_line_compile_error>
straight_line_compiler::compile(const loader::program_image& image) {
    const auto graph = cfg::cfg_builder::build(image.bytes, image.entry_offset());
    if (!graph) return std::unexpected(straight_line_compile_error{std::string{"cannot build CFG: "} + graph.error().message});
    const auto ir = ir::control_flow_lowerer::lower(*graph);
    if (!ir) return std::unexpected(straight_line_compile_error{std::string{"cannot lower IR: "} + ir.error().message});
    if (graph->blocks.empty()) return std::unexpected(straight_line_compile_error{"straight-line compiler found no reachable blocks"});
    translation_state ts;
    ts.state = ts.ssa.entry_state();
    ts.visited_block.assign(graph->blocks.size(), false);
    ts.registers[static_cast<std::size_t>(decoder::register_name::sp)] = 0xfffe;
    if (image.format == loader::executable_format::mz) {
        ts.registers[static_cast<std::size_t>(decoder::register_name::ss)] = image.initial_stack.segment;
        ts.registers[static_cast<std::size_t>(decoder::register_name::sp)] = image.initial_stack.offset;
    }
    if (image.format == loader::executable_format::mz) {
        const auto first_byte = static_cast<std::uint32_t>(image.entry_point.segment) * 16U;
        const auto loaded = ts.memory.write(first_byte, std::span<const std::byte>{image.bytes.data(), image.bytes.size()});
        if (!loaded) return std::unexpected(straight_line_compile_error{std::string{"cannot load MZ image into real-mode memory: "} + loaded.error().message});
    } else {
        const auto loaded = ts.memory.write(0x100, std::span<const std::byte>{image.bytes.data(), image.bytes.size()});
        if (!loaded) return std::unexpected(straight_line_compile_error{std::string{"cannot load COM image into real-mode memory: "} + loaded.error().message});
    }
    const auto exit_code = translate_block(image, *graph, 0, ts);
    if (!exit_code) return std::unexpected(exit_code.error());
    if (ts.writes.empty()) return backend::elf_writer::emit_exit_executable(*exit_code);
    return backend::elf_writer::emit_multi_write_exit_executable(std::span<const backend::write_call>{ts.writes.data(), ts.writes.size()}, *exit_code);
}

std::expected<std::string, straight_line_compile_error>
straight_line_compiler::emit_llvm(const loader::program_image& image) {
    const auto graph = cfg::cfg_builder::build(image.bytes, image.entry_offset());
    if (!graph) return std::unexpected(straight_line_compile_error{std::string{"cannot build CFG: "} + graph.error().message});
    const auto ir = ir::control_flow_lowerer::lower(*graph);
    if (!ir) return std::unexpected(straight_line_compile_error{std::string{"cannot lower IR: "} + ir.error().message});
    if (graph->blocks.empty()) return std::unexpected(straight_line_compile_error{"no reachable blocks"});
    ir::register_ssa_builder ssa;
    auto state = ssa.entry_state();
    decoder::register_values registers{};
    registers[static_cast<std::size_t>(decoder::register_name::sp)] = 0xfffe;
    runtime::real_mode_memory memory;
    if (image.format == loader::executable_format::mz) {
        const auto first_byte = static_cast<std::uint32_t>(image.entry_point.segment) * 16U;
        const auto loaded = memory.write(first_byte, std::span<const std::byte>{image.bytes.data(), image.bytes.size()});
        if (!loaded) return std::unexpected(straight_line_compile_error{std::string{"cannot load MZ: "} + loaded.error().message});
    } else {
        const auto loaded = memory.write(0x100, std::span<const std::byte>{image.bytes.data(), image.bytes.size()});
        if (!loaded) return std::unexpected(straight_line_compile_error{std::string{"cannot load COM: "} + loaded.error().message});
    }
    std::ostringstream out;
    out << "; ModuleID = 'dosrecomp'\nsource_filename = \"dosrecomp\"\n\n";
    out << "define i32 @main() {\nentry:\n";
    std::size_t position = graph->blocks.front().start;
    std::uint16_t exit_code_value = 0;
    bool saw_exit = false;
    while (position < image.bytes.size()) {
        const auto decoded = decoder::instruction_decoder::decode_at(image.bytes, position);
        if (!decoded) return std::unexpected(straight_line_compile_error{std::string{"cannot decode at "} + std::to_string(position) + ": " + decoded.error().message});
        if (decoded->kind == decoder::instruction_kind::interrupt) {
            const auto ah_ssa = state.values[static_cast<std::size_t>(ir::register_id::ah)];
            std::uint16_t ah_const = 0;
            std::size_t cursor = ah_ssa;
            while (cursor < ssa.values().size()) {
                const auto& v = ssa.values()[cursor];
                if (v.constant) { ah_const = *v.constant; break; }
                if (v.inputs.empty()) break;
                cursor = v.inputs.front();
            }
            const auto ax_ssa = state.values[static_cast<std::size_t>(ir::register_id::ax)];
            std::uint16_t ax_const = 0;
            cursor = ax_ssa;
            while (cursor < ssa.values().size()) {
                const auto& v = ssa.values()[cursor];
                if (v.constant) { ax_const = *v.constant; break; }
                if (v.inputs.empty()) break;
                cursor = v.inputs.front();
            }
            const auto ah_value = ah_const != 0 || ah_ssa == ax_ssa ? static_cast<std::uint8_t>(ah_const) : static_cast<std::uint8_t>(ax_const >> 8U);
            if (ah_value != 0x4cU) return std::unexpected(straight_line_compile_error{"non-exit INT 21h not supported in LLVM emit"});
            const auto al_ssa = state.values[static_cast<std::size_t>(ir::register_id::al)];
            std::uint16_t al_const = 0;
            cursor = al_ssa;
            while (cursor < ssa.values().size()) {
                const auto& v = ssa.values()[cursor];
                if (v.constant) { al_const = *v.constant; break; }
                if (v.inputs.empty()) break;
                cursor = v.inputs.front();
            }
            exit_code_value = al_const != 0 || al_ssa == ax_ssa ? static_cast<std::uint8_t>(al_const) : static_cast<std::uint8_t>(ax_const & 0xffU);
            saw_exit = true;
            break;
        }
        const auto effect = semantics::instruction_translator::translate(image.bytes, *decoded, ssa, state, &registers, &memory);
        if (!effect) return std::unexpected(straight_line_compile_error{std::string{"cannot translate at "} + std::to_string(position) + ": " + effect.error().message});
        const auto& value = ssa.values()[effect->ssa_value];
        static const std::array<std::string_view, 18> names{
            "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
            "ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "flags", "any"
        };
        const auto name = std::string(names[static_cast<std::size_t>(value.reg)]) + "_" + std::to_string(value.id);
        if (value.constant) {
            out << "  %" << name << " = add i16 0, " << *value.constant << "\n";
        } else if (value.operation && value.inputs.size() == 2) {
            const auto& left = ssa.values()[value.inputs[0]];
            const auto& right = ssa.values()[value.inputs[1]];
            std::string op;
            switch (*value.operation) {
            case ir::operation_kind::add: op = "add"; break;
            case ir::operation_kind::subtract: op = "sub"; break;
            case ir::operation_kind::bit_and: op = "and"; break;
            case ir::operation_kind::bit_or: op = "or"; break;
            case ir::operation_kind::bit_xor: op = "xor"; break;
            case ir::operation_kind::compare: op = "sub"; break;
            case ir::operation_kind::test: op = "and"; break;
            }
            out << "  %" << name << " = " << op << " i16 %"
                << names[static_cast<std::size_t>(left.reg)] << "_" << left.id
                << ", %" << names[static_cast<std::size_t>(right.reg)] << "_" << right.id << "\n";
        }
        position += decoded->size;
    }
    if (!saw_exit) return std::unexpected(straight_line_compile_error{"straight-line program must terminate with INT 21h"});
    out << "  ret i32 " << static_cast<unsigned>(exit_code_value) << "\n}\n";
    return out.str();
}

} // namespace dosrecomp::compiler