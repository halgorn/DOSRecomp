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
    register_ssa_builder flag_builder;
    auto flag_state = flag_builder.entry_state();
    const auto ax_5 = flag_builder.define_constant(flag_state, register_id::ax, 5);
    const auto ax_3 = flag_builder.define_constant(flag_state, register_id::ax, 3);
    const auto cmp_5_3 = flag_builder.define_operation(flag_state, register_id::flags, operation_kind::compare, {ax_5, ax_3});
    const auto eq_5_5 = flag_builder.define_operation(flag_state, register_id::flags, operation_kind::compare, {ax_5, ax_5});
    const auto ax_neg = flag_builder.define_constant(flag_state, register_id::ax, 0x8000);
    const auto ax_one = flag_builder.define_constant(flag_state, register_id::ax, 1);
    const auto cmp_neg_one = flag_builder.define_operation(flag_state, register_id::flags, operation_kind::compare, {ax_neg, ax_one});
    const auto test_5_3 = flag_builder.define_operation(flag_state, register_id::flags, operation_kind::test, {ax_5, ax_3});
    const auto test_5_2 = flag_builder.define_operation(flag_state, register_id::flags, operation_kind::test, {ax_5, flag_builder.define_constant(flag_state, register_id::ax, 2)});
    const auto gt = flag_builder.resolve_compare_flags(cmp_5_3);
    const auto eq = flag_builder.resolve_compare_flags(eq_5_5);
    const auto ov = flag_builder.resolve_compare_flags(cmp_neg_one);
    const auto tst = flag_builder.resolve_compare_flags(test_5_3);
    const auto tst_zero = flag_builder.resolve_compare_flags(test_5_2);
    if (gt.zero || gt.carry || gt.sign || gt.overflow ||
        !eq.zero || eq.carry || eq.sign || eq.overflow ||
        ov.sign || ov.carry || ov.zero || !ov.overflow ||
        tst.zero || tst.sign || !tst_zero.zero) {
        std::cerr << "failed to resolve compare flag bits from SSA chain\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
