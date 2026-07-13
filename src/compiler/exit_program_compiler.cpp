#include "dosrecomp/compiler/exit_program_compiler.hpp"

#include "dosrecomp/backend/elf_writer.hpp"
#include "dosrecomp/decoder/instruction_decoder.hpp"
#include "dosrecomp/ir/register_ssa.hpp"
#include "dosrecomp/semantics/instruction_translator.hpp"

namespace dosrecomp::compiler {
namespace {
[[nodiscard]] std::expected<std::size_t, compile_error>
skip_nops(const std::vector<std::byte>& code, std::size_t offset) {
    while (offset < code.size()) {
        const auto instruction = decoder::instruction_decoder::decode_at(code, offset);
        if (!instruction) return std::unexpected(compile_error{"cannot decode entry instruction: " + instruction.error().message});
        if (instruction->kind != decoder::instruction_kind::nop) return offset;
        offset += instruction->size;
    }
    return std::unexpected(compile_error{"entry sequence contains only NOP instructions"});
}

[[nodiscard]] bool is_exit_interrupt(const std::vector<std::byte>& code, std::size_t offset) {
    const auto interrupt = decoder::instruction_decoder::decode_at(code, offset);
    return interrupt && interrupt->kind == decoder::instruction_kind::interrupt && interrupt->interrupt_number == 0x21;
}

[[nodiscard]] std::expected<std::uint8_t, compile_error>
extract_exit_code_at(const loader::program_image& image, std::size_t offset) {
    const auto move = decoder::instruction_decoder::decode_at(image.bytes, offset);
    if (!move) return std::unexpected(compile_error{"cannot decode exit instruction: " + move.error().message});
    if (move->kind == decoder::instruction_kind::move_immediate && move->size == 3 &&
        std::to_integer<std::uint8_t>(image.bytes[offset]) == 0xb8 && offset + 3 <= image.bytes.size()) {
        const auto immediate = static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(image.bytes[offset + 1])) |
                               (static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(image.bytes[offset + 2])) << 8U);
        const auto interrupt = skip_nops(image.bytes, offset + move->size);
        if (interrupt && is_exit_interrupt(image.bytes, *interrupt) && (immediate >> 8U) == 0x4cU) {
            return static_cast<std::uint8_t>(immediate);
        }
    }
    return std::unexpected(compile_error{"console output must be followed by MOV AX, 4Cxxh; INT 21h"});
}

[[nodiscard]] std::expected<std::vector<std::byte>, compile_error>
compile_console_program(const loader::program_image& image) {
    if (image.format != loader::executable_format::com) {
        return std::unexpected(compile_error{"INT 21h/AH=09h console compilation currently supports COM images only"});
    }
    const auto entry_result = skip_nops(image.bytes, image.entry_offset());
    if (!entry_result) return std::unexpected(entry_result.error());
    const auto entry = *entry_result;
    if (image.bytes.size() - entry < 5 || std::to_integer<std::uint8_t>(image.bytes[entry]) != 0xba ||
        std::to_integer<std::uint8_t>(image.bytes[entry + 3]) != 0xb4 ||
        std::to_integer<std::uint8_t>(image.bytes[entry + 4]) != 0x09) {
        return std::unexpected(compile_error{"entry is not MOV DX, offset; MOV AH, 09h"});
    }
    const auto dx = static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(image.bytes[entry + 1])) |
                    (static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(image.bytes[entry + 2])) << 8U);
    const auto dx_offset = static_cast<std::size_t>(dx);
    if (dx_offset < 0x100U || dx_offset - 0x100U >= image.bytes.size()) {
        return std::unexpected(compile_error{"INT 21h/AH=09h DX points outside the COM image"});
    }
    const auto interrupt_result = skip_nops(image.bytes, entry + 5);
    if (!interrupt_result || !is_exit_interrupt(image.bytes, *interrupt_result)) {
        return std::unexpected(compile_error{"MOV AH, 09h must be followed by INT 21h"});
    }
    const auto exit_offset = skip_nops(image.bytes, *interrupt_result + 2);
    if (!exit_offset) return std::unexpected(exit_offset.error());
    const auto exit_code = extract_exit_code_at(image, *exit_offset);
    if (!exit_code) return std::unexpected(exit_code.error());
    const auto string_begin = dx_offset - 0x100U;
    std::vector<std::byte> message;
    for (std::size_t cursor = string_begin; cursor < image.bytes.size(); ++cursor) {
        if (image.bytes[cursor] == std::byte{'$'}) return backend::elf_writer::emit_write_exit_executable(message, *exit_code);
        message.push_back(image.bytes[cursor]);
    }
    return std::unexpected(compile_error{"INT 21h/AH=09h string is missing its '$' terminator"});
}
}

std::expected<std::uint8_t, compile_error>
exit_program_compiler::extract_exit_code(const loader::program_image& image) {
    const auto entry_result = skip_nops(image.bytes, image.entry_offset());
    if (!entry_result) return std::unexpected(entry_result.error());
    const auto entry = *entry_result;
    auto combine_byte_exit = [&]() -> std::expected<std::uint16_t, compile_error> {
        std::uint8_t ah_value = 0;
        std::uint8_t al_value = 0;
        std::size_t cursor = entry;
        bool saw_ah = false;
        bool saw_al = false;
        while (cursor < image.bytes.size()) {
            const auto inst = decoder::instruction_decoder::decode_at(image.bytes, cursor);
            if (!inst) return std::unexpected(compile_error{"cannot decode byte-form entry: " + inst.error().message});
            if (inst->kind == decoder::instruction_kind::interrupt) break;
            if (inst->kind != decoder::instruction_kind::move_immediate || inst->size != 2 ||
                inst->operands[0].kind != decoder::operand_kind::reg || inst->operands[0].width != decoder::operand_width::byte) {
                return std::unexpected(compile_error{"byte-form entry must be MOV AH, imm and MOV AL, imm"});
            }
            const auto imm = inst->operands[1].immediate & 0xffU;
            if (inst->operands[0].reg == decoder::register_name::ah) { ah_value = static_cast<std::uint8_t>(imm); saw_ah = true; }
            else if (inst->operands[0].reg == decoder::register_name::al) { al_value = static_cast<std::uint8_t>(imm); saw_al = true; }
            else return std::unexpected(compile_error{"byte-form entry must use AH or AL"});
            cursor += inst->size;
        }
        if (!saw_ah || !saw_al) return std::unexpected(compile_error{"byte-form entry must set both AH and AL"});
        return static_cast<std::uint16_t>(ah_value << 8U | al_value);
    };
    const auto move = decoder::instruction_decoder::decode_at(image.bytes, entry);
    if (!move) return std::unexpected(compile_error{"cannot decode entry instruction: " + move.error().message});
    ir::register_ssa_builder ssa;
    auto state = ssa.entry_state();
    const auto interrupt_result = skip_nops(image.bytes, entry + move->size);
    if (!interrupt_result) return std::unexpected(interrupt_result.error());
    const auto interrupt_offset = *interrupt_result;
    const auto effect = semantics::instruction_translator::translate(image.bytes, *move, ssa, state);
    if (effect) {
        if (move->kind == decoder::instruction_kind::move_immediate && move->operands[0].width == decoder::operand_width::byte) {
            const auto combined = combine_byte_exit();
            if (!combined) return std::unexpected(combined.error());
            auto exit_value = *combined;
            auto final_interrupt = interrupt_offset;
            if (!is_exit_interrupt(image.bytes, final_interrupt)) {
                const auto following = skip_nops(image.bytes, final_interrupt + 2);
                if (!following) return std::unexpected(following.error());
                final_interrupt = *following;
            }
            if (!is_exit_interrupt(image.bytes, final_interrupt) || (exit_value >> 8U) != 0x4cU) {
                return std::unexpected(compile_error{"entry sequence does not terminate through INT 21h exit"});
            }
            return static_cast<std::uint8_t>(exit_value);
        }
        if (effect->destination != ir::register_id::ax || !effect->immediate) {
            return std::unexpected(compile_error{"INT 21h exit requires MOV AX, 4Cxxh"});
        }
        auto exit_value = *effect->immediate;
        auto final_interrupt = interrupt_offset;
        if (!is_exit_interrupt(image.bytes, final_interrupt)) {
            const auto arithmetic = decoder::instruction_decoder::decode_at(image.bytes, final_interrupt);
            if (!arithmetic || arithmetic->kind != decoder::instruction_kind::arithmetic || arithmetic->operand_count != 2 ||
                arithmetic->operands[0].reg != decoder::register_name::ax || arithmetic->operands[1].width != decoder::operand_width::word) {
                return std::unexpected(compile_error{"entry sequence does not terminate through INT 21h"});
            }
            const auto immediate = arithmetic->operands[1].immediate;
            switch (arithmetic->alu) {
            case decoder::alu_operation::add: exit_value = static_cast<std::uint16_t>(exit_value + immediate); break;
            case decoder::alu_operation::subtract: exit_value = static_cast<std::uint16_t>(exit_value - immediate); break;
            case decoder::alu_operation::bit_and: exit_value = static_cast<std::uint16_t>(exit_value & immediate); break;
            case decoder::alu_operation::bit_or: exit_value = static_cast<std::uint16_t>(exit_value | immediate); break;
            case decoder::alu_operation::bit_xor: exit_value = static_cast<std::uint16_t>(exit_value ^ immediate); break;
            default: return std::unexpected(compile_error{"exit arithmetic operation is unsupported"});
            }
            const auto following = skip_nops(image.bytes, final_interrupt + arithmetic->size);
            if (!following) return std::unexpected(following.error());
            final_interrupt = *following;
        }
        if (!is_exit_interrupt(image.bytes, final_interrupt) || (exit_value >> 8U) != 0x4cU) {
            return std::unexpected(compile_error{"entry sequence does not terminate through INT 21h exit"});
        }
        return static_cast<std::uint8_t>(exit_value);
    }

    const auto combined = combine_byte_exit();
    if (!combined) return std::unexpected(combined.error());
    return static_cast<std::uint8_t>(*combined);
}

std::expected<std::vector<std::byte>, compile_error>
exit_program_compiler::compile(const loader::program_image& image) {
    if (const auto console = compile_console_program(image); console) return *console;
    const auto exit_code = extract_exit_code(image);
    if (!exit_code) return std::unexpected(exit_code.error());
    return backend::elf_writer::emit_exit_executable(*exit_code);
}

std::expected<std::string, compile_error>
exit_program_compiler::emit_llvm(const loader::program_image& image) {
    const auto exit_code = extract_exit_code(image);
    if (!exit_code) return std::unexpected(exit_code.error());
    return "; ModuleID = 'dosrecomp'\nsource_filename = \"dosrecomp\"\n\ndefine i32 @main() {\n  ret i32 " +
        std::to_string(static_cast<unsigned>(*exit_code)) + "\n}\n";
}

} // namespace dosrecomp::compiler
