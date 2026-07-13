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
    if (!effect || effect->destination != dosrecomp::ir::register_id::ax || effect->immediate != 0x1234 ||
        builder.values().size() != 10) {
        std::cerr << "failed to translate MOV AX, imm16\n";
        return EXIT_FAILURE;
    }
    const auto unsupported = dosrecomp::semantics::instruction_translator::translate({b(0x90)},
        {dosrecomp::decoder::instruction_kind::nop, 0, 1, 0, 0}, builder, state);
    return unsupported ? EXIT_FAILURE : EXIT_SUCCESS;
}
