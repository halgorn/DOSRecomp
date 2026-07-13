#include "dosrecomp/compiler/exit_program_compiler.hpp"

#include "dosrecomp/backend/elf_writer.hpp"
#include "dosrecomp/decoder/instruction_decoder.hpp"
#include "dosrecomp/ir/register_ssa.hpp"
#include "dosrecomp/semantics/instruction_translator.hpp"

namespace dosrecomp::compiler {
std::expected<std::vector<std::byte>, compile_error>
exit_program_compiler::compile(const loader::program_image& image) {
    const auto entry = image.entry_offset();
    const auto move = decoder::instruction_decoder::decode_at(image.bytes, entry);
    if (!move) return std::unexpected(compile_error{"cannot decode entry instruction: " + move.error().message});
    ir::register_ssa_builder ssa;
    auto state = ssa.entry_state();
    const auto effect = semantics::instruction_translator::translate(image.bytes, *move, ssa, state);
    if (!effect) return std::unexpected(compile_error{"cannot translate entry instruction: " + effect.error().message});
    const auto interrupt_offset = entry + move->size;
    const auto interrupt = decoder::instruction_decoder::decode_at(image.bytes, interrupt_offset);
    if (!interrupt || interrupt->kind != decoder::instruction_kind::interrupt || interrupt->interrupt_number != 0x21) {
        return std::unexpected(compile_error{"entry sequence does not terminate through INT 21h"});
    }
    if (effect->destination != ir::register_id::ax || (effect->immediate >> 8U) != 0x4cU) {
        return std::unexpected(compile_error{"INT 21h exit requires MOV AX, 4Cxxh"});
    }
    return backend::elf_writer::emit_exit_executable(static_cast<std::uint8_t>(effect->immediate));
}

} // namespace dosrecomp::compiler

