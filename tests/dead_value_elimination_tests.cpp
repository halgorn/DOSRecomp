#include "dosrecomp/optimizer/dead_value_elimination.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    using namespace dosrecomp::ir;
    register_ssa_builder builder;
    auto state = builder.entry_state();
    const auto used = builder.define_constant(state, register_id::ax, 1);
    const auto dead = builder.define_constant(state, register_id::bx, 2);
    const auto result = dosrecomp::optimizer::dead_value_elimination::analyze(builder.values(), {used});
    if (!result || !(*result)[used] || (*result)[dead] || (*result)[0]) {
        std::cerr << "failed dead value liveness analysis\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

