#include "dosrecomp/semantics/instruction_translator.hpp"

#include <cstdlib>
#include <iostream>

namespace { std::byte b(unsigned value) { return static_cast<std::byte>(value); } }

int main() {
    const std::vector<std::byte> code{b(0xb8), b(0x34), b(0x12)};
    const auto decoded = dosrecomp::decoder::instruction_decoder::decode_at(code, 0);
    if (!decoded) return EXIT_FAILURE;
    dosrecomp::ir::register_ssa_builder builder;
    auto state = builder.entry_state();
    const auto effect = dosrecomp::semantics::instruction_translator::translate(code, *decoded, builder, state);
    const std::vector<std::byte> register_code{b(0x8b), b(0xc3)};
    const auto register_decoded = dosrecomp::decoder::instruction_decoder::decode_at(register_code, 0);
    const auto register_effect = register_decoded ? dosrecomp::semantics::instruction_translator::translate(register_code, *register_decoded, builder, state)
                                                  : std::expected<dosrecomp::semantics::semantic_effect, dosrecomp::semantics::translation_error>{std::unexpected(dosrecomp::semantics::translation_error{"decode failed"})};
    if (!effect || effect->destination != dosrecomp::ir::register_id::ax || effect->immediate != 0x1234 || !register_effect || register_effect->immediate ||
        builder.values().size() != 11) {
        std::cerr << "failed to translate MOV AX, imm16\n";
        return EXIT_FAILURE;
    }
    const auto unsupported = dosrecomp::semantics::instruction_translator::translate({b(0x90)},
        {dosrecomp::decoder::instruction_kind::nop, 0, 1, 0, 0}, builder, state);
    return unsupported ? EXIT_FAILURE : EXIT_SUCCESS;
}
