#include "dosrecomp/ir/register_ssa.hpp"

#include <utility>

namespace dosrecomp::ir {
namespace {
constexpr std::size_t index_of(register_id reg) { return static_cast<std::size_t>(reg); }
}

register_ssa_builder::register_ssa_builder() {
    values_.reserve(index_of(register_id::count));
    for (std::size_t index = 0; index < index_of(register_id::count); ++index) {
        values_.push_back({.id = index, .reg = static_cast<register_id>(index), .kind = value_kind::entry, .inputs = {}, .constant = std::nullopt, .operation = std::nullopt});
    }
}

register_state register_ssa_builder::entry_state() const noexcept {
    register_state state;
    for (std::size_t index = 0; index < state.values.size(); ++index) state.values[index] = index;
    return state;
}

std::size_t register_ssa_builder::define(register_state& state, register_id reg, std::vector<std::size_t> inputs) {
    const auto id = values_.size();
    values_.push_back({.id = id, .reg = reg, .kind = value_kind::definition, .inputs = std::move(inputs), .constant = std::nullopt, .operation = std::nullopt});
    state.values[index_of(reg)] = id;
    return id;
}

std::size_t register_ssa_builder::define_constant(register_state& state, register_id reg, std::uint16_t value) {
    const auto id = values_.size();
    values_.push_back({.id = id, .reg = reg, .kind = value_kind::definition, .inputs = {}, .constant = value, .operation = std::nullopt});
    state.values[index_of(reg)] = id;
    return id;
}

std::size_t register_ssa_builder::constant(register_id reg, std::uint16_t value) {
    const auto id = values_.size();
    values_.push_back({.id = id, .reg = reg, .kind = value_kind::definition, .inputs = {}, .constant = value, .operation = std::nullopt});
    return id;
}

std::size_t register_ssa_builder::define_operation(register_state& state, register_id reg, operation_kind operation,
                                                    std::vector<std::size_t> inputs) {
    const auto id = values_.size();
    values_.push_back({.id = id, .reg = reg, .kind = value_kind::operation, .inputs = std::move(inputs), .constant = std::nullopt, .operation = operation});
    state.values[index_of(reg)] = id;
    return id;
}

register_state register_ssa_builder::merge(const register_state& first, const register_state& second) {
    auto merged = first;
    for (std::size_t index = 0; index < merged.values.size(); ++index) {
        if (first.values[index] == second.values[index]) continue;
        const auto id = values_.size();
        values_.push_back({.id = id, .reg = static_cast<register_id>(index), .kind = value_kind::phi,
            .inputs = {first.values[index], second.values[index]}, .constant = std::nullopt, .operation = std::nullopt});
        merged.values[index] = id;
    }
    return merged;
}

const std::vector<ssa_value>& register_ssa_builder::values() const noexcept { return values_; }

namespace {
[[nodiscard]] std::optional<std::uint16_t> resolve_constant(const std::vector<ssa_value>& values, std::size_t ssa_value) {
    std::size_t current = ssa_value;
    while (true) {
        const auto& value = values[current];
        if (value.constant) return value.constant;
        if (value.operation && value.inputs.size() == 2) {
            const auto& first = values[value.inputs[0]];
            const auto& second = values[value.inputs[1]];
            if (first.constant && second.constant) {
                switch (*value.operation) {
                case operation_kind::add: return static_cast<std::uint16_t>(*first.constant + *second.constant);
                case operation_kind::subtract: return static_cast<std::uint16_t>(*first.constant - *second.constant);
                case operation_kind::bit_and: return static_cast<std::uint16_t>(*first.constant & *second.constant);
                case operation_kind::bit_or: return static_cast<std::uint16_t>(*first.constant | *second.constant);
                case operation_kind::bit_xor: return static_cast<std::uint16_t>(*first.constant ^ *second.constant);
                default: return std::nullopt;
                }
            }
        }
        if (value.inputs.empty()) return std::nullopt;
        current = value.inputs.front();
    }
}
}

compare_flags register_ssa_builder::resolve_compare_flags(std::size_t flags_ssa) const {
    compare_flags result{};
    std::size_t current = flags_ssa;
    while (true) {
        const auto& value = values_[current];
        if (value.operation == operation_kind::compare && value.inputs.size() == 2) {
            const auto left = resolve_constant(values_, value.inputs[0]);
            const auto right = resolve_constant(values_, value.inputs[1]);
            if (left && right) {
                const auto left_val = static_cast<std::uint16_t>(*left);
                const auto right_val = static_cast<std::uint16_t>(*right);
                const auto result16 = static_cast<std::uint16_t>(left_val - right_val);
                result.zero = result16 == 0;
                result.sign = (result16 & 0x8000U) != 0;
                result.carry = left_val < right_val;
                const auto left_signed = static_cast<std::int16_t>(left_val);
                const auto right_signed = static_cast<std::int16_t>(right_val);
                const auto result_signed = static_cast<std::int16_t>(result16);
                const bool sign_change = (left_signed < 0) != (right_signed < 0);
                result.overflow = sign_change && (result_signed < 0) != (left_signed < 0);
                return result;
            }
        }
        if (value.operation == operation_kind::test && value.inputs.size() == 2) {
            const auto left = resolve_constant(values_, value.inputs[0]);
            const auto right = resolve_constant(values_, value.inputs[1]);
            if (left && right) {
                const auto masked = static_cast<std::uint16_t>(*left & *right);
                result.zero = masked == 0;
                result.sign = (masked & 0x8000U) != 0;
                result.carry = false;
                result.overflow = false;
                return result;
            }
        }
        if (value.inputs.empty()) return result;
        current = value.inputs.front();
    }
}

} // namespace dosrecomp::ir
