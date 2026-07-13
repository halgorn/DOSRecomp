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
}

std::expected<std::uint8_t, compile_error>
exit_program_compiler::extract_exit_code(const loader::program_image& image) {
    const auto entry_result = skip_nops(image.bytes, image.entry_offset());
    if (!entry_result) return std::unexpected(entry_result.error());
    const auto entry = *entry_result;
    const auto move = decoder::instruction_decoder::decode_at(image.bytes, entry);
    if (!move) return std::unexpected(compile_error{"cannot decode entry instruction: " + move.error().message});
    ir::register_ssa_builder ssa;
    auto state = ssa.entry_state();
    const auto interrupt_result = skip_nops(image.bytes, entry + move->size);
    if (!interrupt_result) return std::unexpected(interrupt_result.error());
    const auto interrupt_offset = *interrupt_result;
    const auto effect = semantics::instruction_translator::translate(image.bytes, *move, ssa, state);
    if (effect) {
        if (!is_exit_interrupt(image.bytes, interrupt_offset)) return std::unexpected(compile_error{"entry sequence does not terminate through INT 21h"});
        if (effect->destination != ir::register_id::ax || (effect->immediate >> 8U) != 0x4cU) {
            return std::unexpected(compile_error{"INT 21h exit requires MOV AX, 4Cxxh"});
        }
        return static_cast<std::uint8_t>(effect->immediate);
    }

    const auto first_opcode = std::to_integer<std::uint8_t>(image.bytes[entry]);
    if (first_opcode != 0xb4 || move->size != 2 || image.bytes.size() - entry < 2 ||
        std::to_integer<std::uint8_t>(image.bytes[entry + 1]) != 0x4c) {
        return std::unexpected(compile_error{"cannot translate entry instruction: " + effect.error().message});
    }
    const auto low_move = decoder::instruction_decoder::decode_at(image.bytes, interrupt_offset);
    const auto byte_interrupt_result = low_move ? skip_nops(image.bytes, interrupt_offset + low_move->size)
                                                : std::expected<std::size_t, compile_error>{std::unexpected(compile_error{"cannot decode MOV AL, immediate"})};
    if (!low_move || low_move->size != 2 || interrupt_offset + 2 > image.bytes.size() ||
        std::to_integer<std::uint8_t>(image.bytes[interrupt_offset]) != 0xb0 ||
        !byte_interrupt_result || !is_exit_interrupt(image.bytes, *byte_interrupt_result)) {
        return std::unexpected(compile_error{"INT 21h exit requires MOV AL, immediate after MOV AH, 4Ch"});
    }
    return std::to_integer<std::uint8_t>(image.bytes[interrupt_offset + 1]);
}

std::expected<std::vector<std::byte>, compile_error>
exit_program_compiler::compile(const loader::program_image& image) {
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
