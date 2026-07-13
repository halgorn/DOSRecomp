#include "dosrecomp/backend/elf_writer.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    const auto image = dosrecomp::backend::elf_writer::emit_exit_executable(7);
    const auto byte = [&](std::size_t index) { return std::to_integer<unsigned char>(image[index]); };
    if (image.size() != 132 || byte(0) != 0x7f || byte(1) != 'E' || byte(4) != 2 || byte(18) != 62 ||
        byte(120) != 0xb8 || byte(125) != 0xbf || byte(126) != 7 || byte(130) != 0x0f || byte(131) != 0x05) {
        std::cerr << "failed ELF64 image emission\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

