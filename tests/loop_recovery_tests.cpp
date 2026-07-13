#include "dosrecomp/analysis/loop_recovery.hpp"

#include <cstdlib>
#include <iostream>

namespace { std::byte b(unsigned value) { return static_cast<std::byte>(value); } }

int main() {
    const std::vector<std::byte> code{b(0xeb), b(0xfe)};
    const auto graph = dosrecomp::cfg::cfg_builder::build(code, 0);
    if (!graph) return EXIT_FAILURE;
    const auto loops = dosrecomp::analysis::loop_recovery::recover(code, *graph);
    if (!loops || loops->size() != 1 || (*loops)[0].header != 0 || (*loops)[0].latch != 0) {
        std::cerr << "failed loop recovery\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

