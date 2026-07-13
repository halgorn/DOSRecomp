#include "dosrecomp/ir/control_flow_ir.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    const dosrecomp::cfg::control_flow_graph graph{{
        {.start = 0, .end = 2, .successors = {2, 4}, .condition = dosrecomp::decoder::branch_condition::equal},
        {.start = 2, .end = 4, .successors = {}},
        {.start = 4, .end = 6, .successors = {}},
    }};
    const auto result = dosrecomp::ir::control_flow_lowerer::lower(graph);
    if (!result || result->blocks.size() != 3 ||
        result->blocks[0].terminator != dosrecomp::ir::terminator_kind::branch ||
        result->blocks[0].successors != std::vector<std::size_t>{1, 2} ||
        result->blocks[0].condition != dosrecomp::decoder::branch_condition::equal) {
        std::cerr << "failed to lower CFG to IR\n";
        return EXIT_FAILURE;
    }
    const auto invalid = dosrecomp::ir::control_flow_lowerer::lower({{{.start = 0, .end = 1, .successors = {9}}}});
    return invalid ? EXIT_FAILURE : EXIT_SUCCESS;
}
