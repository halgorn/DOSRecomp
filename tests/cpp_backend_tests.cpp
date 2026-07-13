#include "dosrecomp/compiler/cpp_backend.hpp"

#include <cstdlib>
#include <iostream>

namespace { std::byte b(unsigned value) { return static_cast<std::byte>(value); } }

int main() {
    const auto loaded = dosrecomp::loader::binary_loader::load_bytes({b(0xb8), b(7), b(0x4c), b(0xcd), b(0x21)});
    if (!loaded) return EXIT_FAILURE;
    const auto cpp = dosrecomp::compiler::cpp_backend::emit(*loaded);
    if (!cpp) {
        std::cerr << "failed to emit C++: " << cpp.error().message << "\n";
        return EXIT_FAILURE;
    }
    if (cpp->find("u16_ax = 0x4c07") == std::string::npos ||
        cpp->find("dos_int21(") == std::string::npos ||
        cpp->find("return al()") == std::string::npos) {
        std::cerr << "C++ backend output missing expected statements\n";
        return EXIT_FAILURE;
    }
    const auto failed = dosrecomp::compiler::cpp_backend::emit({
        .format = dosrecomp::loader::executable_format::com, .bytes = {b(0xff)},
        .entry_point = {0, 0x100}, .initial_stack = {}, .relocations = {}});
    if (failed) {
        std::cerr << "C++ backend should reject unsupported opcodes\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}