#include "dosrecomp/compiler/exit_program_compiler.hpp"

#include <cstdlib>
#include <iostream>

namespace { std::byte b(unsigned value) { return static_cast<std::byte>(value); } }

int main() {
    const auto loaded = dosrecomp::loader::binary_loader::load_bytes({b(0xb8), b(7), b(0x4c), b(0xcd), b(0x21)});
    if (!loaded) return EXIT_FAILURE;
    const auto elf = dosrecomp::compiler::exit_program_compiler::compile(*loaded);
    const auto unsupported = dosrecomp::compiler::exit_program_compiler::compile({
        .format = dosrecomp::loader::executable_format::com, .bytes = {b(0x90)},
        .entry_point = {0, 0x100}, .initial_stack = {}, .relocations = {}});
    if (!elf || elf->size() != 132 || std::to_integer<unsigned char>((*elf)[126]) != 7 || unsupported) {
        std::cerr << "failed end-to-end exit recompilation\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
