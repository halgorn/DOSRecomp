#include "dosrecomp/optimizer/constant_propagation.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    using namespace dosrecomp::ir;
    register_ssa_builder builder;
    auto left = builder.entry_state();
    auto right = builder.entry_state();
    const auto left_constant = builder.define_constant(left, register_id::ax, 42);
    const auto right_constant = builder.define_constant(right, register_id::ax, 42);
    const auto merged = builder.merge(left, right);
    const auto literal = builder.constant(register_id::ax, 1);
    const auto sum = builder.define_operation(left, register_id::ax, operation_kind::add, {left_constant, literal});
    const auto result = dosrecomp::optimizer::constant_propagation::analyze(builder.values());
    const auto phi = merged.values[static_cast<std::size_t>(register_id::ax)];
    if (!result || left_constant == right_constant || !(*result)[phi] || *(*result)[phi] != 42 || !(*result)[sum] || *(*result)[sum] != 43) {
        std::cerr << "failed phi constant propagation\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
