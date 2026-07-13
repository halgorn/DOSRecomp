#include "dosrecomp/ir/register_ssa.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    using namespace dosrecomp::ir;
    register_ssa_builder builder;
    auto left = builder.entry_state();
    auto right = builder.entry_state();
    const auto left_ax = builder.define(left, register_id::ax);
    const auto right_ax = builder.define(right, register_id::ax, {left_ax});
    const auto sum = builder.define_operation(left, register_id::ax, operation_kind::add, {left_ax, right_ax});
    const auto merged = builder.merge(left, right);
    const auto phi = merged.values[static_cast<std::size_t>(register_id::ax)];
    const auto& values = builder.values();
    if (values.size() != 13 || values[sum].kind != value_kind::operation || values[sum].operation != operation_kind::add ||
        values[phi].kind != value_kind::phi ||
        values[phi].inputs != std::vector<std::size_t>{sum, right_ax} ||
        merged.values[static_cast<std::size_t>(register_id::bx)] != 1) {
        std::cerr << "failed to construct register SSA phi\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
