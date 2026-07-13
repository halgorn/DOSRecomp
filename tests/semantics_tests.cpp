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
    const std::vector<std::byte> add_code{b(0x05), b(1), b(0)};
    const auto add_decoded = dosrecomp::decoder::instruction_decoder::decode_at(add_code, 0);
    const auto add_effect = add_decoded ? dosrecomp::semantics::instruction_translator::translate(add_code, *add_decoded, builder, state)
                                        : std::expected<dosrecomp::semantics::semantic_effect, dosrecomp::semantics::translation_error>{std::unexpected(dosrecomp::semantics::translation_error{"decode failed"})};
    const std::vector<std::byte> compare_code{b(0x3d), b(0), b(0)};
    const auto compare_decoded = dosrecomp::decoder::instruction_decoder::decode_at(compare_code, 0);
    const auto compare_effect = compare_decoded ? dosrecomp::semantics::instruction_translator::translate(compare_code, *compare_decoded, builder, state)
                                                : std::expected<dosrecomp::semantics::semantic_effect, dosrecomp::semantics::translation_error>{std::unexpected(dosrecomp::semantics::translation_error{"decode failed"})};
    const std::vector<std::byte> test_code{b(0xa9), b(1), b(0)};
    const auto test_decoded = dosrecomp::decoder::instruction_decoder::decode_at(test_code, 0);
    const auto test_effect = test_decoded ? dosrecomp::semantics::instruction_translator::translate(test_code, *test_decoded, builder, state)
                                          : std::expected<dosrecomp::semantics::semantic_effect, dosrecomp::semantics::translation_error>{std::unexpected(dosrecomp::semantics::translation_error{"decode failed"})};
    if (!effect || effect->destination != dosrecomp::ir::register_id::ax || effect->immediate != 0x1234 || !register_effect || register_effect->immediate || !add_effect || !compare_effect || !test_effect ||
        compare_effect->destination != dosrecomp::ir::register_id::flags || builder.values()[add_effect->ssa_value].operation != dosrecomp::ir::operation_kind::add ||
        builder.values()[compare_effect->ssa_value].operation != dosrecomp::ir::operation_kind::compare ||
        builder.values()[test_effect->ssa_value].operation != dosrecomp::ir::operation_kind::test || builder.values().size() != 17) {
        std::cerr << "failed to translate MOV AX, imm16\n";
        return EXIT_FAILURE;
    }
    const auto unsupported = dosrecomp::semantics::instruction_translator::translate({b(0x90)},
        {dosrecomp::decoder::instruction_kind::nop, 0, 1, 0, 0}, builder, state);
    return unsupported ? EXIT_FAILURE : EXIT_SUCCESS;
}
