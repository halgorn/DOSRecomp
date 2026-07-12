#include "dosrecomp/cfg/cfg_builder.hpp"

#include <cstdlib>
#include <iostream>

namespace {
std::byte b(unsigned value) { return static_cast<std::byte>(value); }
bool expect(bool condition, const char* description) {
    if (!condition) std::cerr << "FAILED: " << description << '\n';
    return condition;
}
}

int main() {
    // JE +2; MOV AX,1; RET; MOV AX,2; RET
    const std::vector<std::byte> code{b(0x74), b(0x04), b(0xb8), b(1), b(0), b(0xc3), b(0xb8), b(2), b(0), b(0xc3)};
    const auto graph = dosrecomp::cfg::cfg_builder::build(code, 0);
    const auto bad_target = dosrecomp::cfg::cfg_builder::build({b(0xeb), b(0x7f)}, 0);
    const bool passed = expect(graph.has_value(), "build branch graph") &&
        expect(graph->blocks.size() == 3, "recover three reachable blocks") &&
        expect(graph->blocks[0].successors.size() == 2, "recover both conditional edges") &&
        expect(!bad_target, "reject out-of-range target");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}

