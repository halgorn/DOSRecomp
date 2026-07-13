#include "dosrecomp/compiler/exit_program_compiler.hpp"

#include "dosrecomp/backend/elf_writer.hpp"
#include "dosrecomp/decoder/instruction_decoder.hpp"
#include "dosrecomp/ir/register_ssa.hpp"
#include "dosrecomp/semantics/instruction_translator.hpp"

namespace dosrecomp::compiler {
namespace {
[[nodiscard]] bool is_exit_interrupt(const std::vector<std::byte>& code, std::size_t offset) {
    const auto interrupt = decoder::instruction_decoder::decode_at(code, offset);
    return interrupt && interrupt->kind == decoder::instruction_kind::interrupt && interrupt->interrupt_number == 0x21;
}
}

std::expected<std::uint8_t, compile_error>
exit_program_compiler::extract_exit_code(const loader::program_image& image) {
    const auto entry = image.entry_offset();
    const auto move = decoder::instruction_decoder::decode_at(image.bytes, entry);
    if (!move) return std::unexpected(compile_error{"cannot decode entry instruction: " + move.error().message});
    ir::register_ssa_builder ssa;
    auto state = ssa.entry_state();
    const auto interrupt_offset = entry + move->size;
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
    if (!low_move || low_move->size != 2 || interrupt_offset + 2 > image.bytes.size() ||
        std::to_integer<std::uint8_t>(image.bytes[interrupt_offset]) != 0xb0 ||
        !is_exit_interrupt(image.bytes, interrupt_offset + low_move->size)) {
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

} // namespace dosrecomp::compiler
