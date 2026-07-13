#include "dosrecomp/runtime/conventional_memory.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    dosrecomp::runtime::conventional_memory memory(10);
    const auto first = memory.allocate(3);
    const auto second = memory.allocate(4);
    const auto exhausted = memory.allocate(4);
    const auto released = first ? memory.release(*first) : std::expected<void, dosrecomp::runtime::memory_error>{std::unexpected(dosrecomp::runtime::memory_error{"allocation failed"})};
    const auto merged = second ? memory.release(*second) : std::expected<void, dosrecomp::runtime::memory_error>{std::unexpected(dosrecomp::runtime::memory_error{"allocation failed"})};
    if (!first || !second || *first != 0 || *second != 3 || exhausted || !released || !merged || memory.largest_free_block() != 10) {
        std::cerr << "failed conventional memory allocation\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

