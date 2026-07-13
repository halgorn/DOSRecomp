#include "dosrecomp/compiler/exit_program_compiler.hpp"

#include <cstdlib>
#include <iostream>

namespace { std::byte b(unsigned value) { return static_cast<std::byte>(value); } }

int main() {
    const auto loaded = dosrecomp::loader::binary_loader::load_bytes({b(0xb8), b(7), b(0x4c), b(0xcd), b(0x21)});
    if (!loaded) return EXIT_FAILURE;
    const auto elf = dosrecomp::compiler::exit_program_compiler::compile(*loaded);
    const auto exit_code = dosrecomp::compiler::exit_program_compiler::extract_exit_code(*loaded);
    const auto llvm = dosrecomp::compiler::exit_program_compiler::emit_llvm(*loaded);
    const auto unsupported = dosrecomp::compiler::exit_program_compiler::compile({
        .format = dosrecomp::loader::executable_format::com, .bytes = {b(0x90)},
        .entry_point = {0, 0x100}, .initial_stack = {}, .relocations = {}});
    std::vector<std::byte> mz(37, b(0));
    mz[0] = b('M'); mz[1] = b('Z'); mz[2] = b(37); mz[4] = b(1); mz[8] = b(2);
    mz[32] = b(0xb8); mz[33] = b(9); mz[34] = b(0x4c); mz[35] = b(0xcd); mz[36] = b(0x21);
    const auto loaded_mz = dosrecomp::loader::binary_loader::load_bytes(mz);
    const auto mz_elf = loaded_mz ? dosrecomp::compiler::exit_program_compiler::compile(*loaded_mz)
                                  : std::expected<std::vector<std::byte>, dosrecomp::compiler::compile_error>{std::unexpected(dosrecomp::compiler::compile_error{"MZ load failed"})};
    const auto byte_form = dosrecomp::loader::binary_loader::load_bytes({b(0xb4), b(0x4c), b(0xb0), b(3), b(0xcd), b(0x21)});
    const auto byte_elf = byte_form ? dosrecomp::compiler::exit_program_compiler::compile(*byte_form)
                                    : std::expected<std::vector<std::byte>, dosrecomp::compiler::compile_error>{std::unexpected(dosrecomp::compiler::compile_error{"byte form load failed"})};
    const auto reversed_byte_form = dosrecomp::loader::binary_loader::load_bytes({b(0xb0), b(4), b(0xb4), b(0x4c), b(0xcd), b(0x21)});
    const auto reversed_byte_elf = reversed_byte_form ? dosrecomp::compiler::exit_program_compiler::compile(*reversed_byte_form)
                                                       : std::expected<std::vector<std::byte>, dosrecomp::compiler::compile_error>{std::unexpected(dosrecomp::compiler::compile_error{"reversed byte form load failed"})};
    const auto padded = dosrecomp::loader::binary_loader::load_bytes({b(0x90), b(0xb8), b(5), b(0x4c), b(0x90), b(0xcd), b(0x21)});
    const auto padded_elf = padded ? dosrecomp::compiler::exit_program_compiler::compile(*padded)
                                   : std::expected<std::vector<std::byte>, dosrecomp::compiler::compile_error>{std::unexpected(dosrecomp::compiler::compile_error{"padded load failed"})};
    if (!elf || !exit_code || !llvm || llvm->find("ret i32 7") == std::string::npos || *exit_code != 7 || elf->size() != 132 || std::to_integer<unsigned char>((*elf)[126]) != 7 ||
        !byte_elf || std::to_integer<unsigned char>((*byte_elf)[126]) != 3 ||
        !reversed_byte_elf || std::to_integer<unsigned char>((*reversed_byte_elf)[126]) != 4 ||
        !padded_elf || std::to_integer<unsigned char>((*padded_elf)[126]) != 5 ||
        !mz_elf || std::to_integer<unsigned char>((*mz_elf)[126]) != 9 || unsupported) {
        std::cerr << "failed end-to-end exit recompilation\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
