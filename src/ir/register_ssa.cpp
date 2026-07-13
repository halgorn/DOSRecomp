#include "dosrecomp/ir/register_ssa.hpp"

#include <utility>

namespace dosrecomp::ir {
namespace {
constexpr std::size_t index_of(register_id reg) { return static_cast<std::size_t>(reg); }
}

register_ssa_builder::register_ssa_builder() {
    values_.reserve(index_of(register_id::count));
    for (std::size_t index = 0; index < index_of(register_id::count); ++index) {
        values_.push_back({.id = index, .reg = static_cast<register_id>(index), .kind = value_kind::entry, .inputs = {}});
    }
}

register_state register_ssa_builder::entry_state() const noexcept {
    register_state state;
    for (std::size_t index = 0; index < state.values.size(); ++index) state.values[index] = index;
    return state;
}

std::size_t register_ssa_builder::define(register_state& state, register_id reg, std::vector<std::size_t> inputs) {
    const auto id = values_.size();
    values_.push_back({.id = id, .reg = reg, .kind = value_kind::definition, .inputs = std::move(inputs)});
    state.values[index_of(reg)] = id;
    return id;
}

register_state register_ssa_builder::merge(const register_state& first, const register_state& second) {
    auto merged = first;
    for (std::size_t index = 0; index < merged.values.size(); ++index) {
        if (first.values[index] == second.values[index]) continue;
        const auto id = values_.size();
        values_.push_back({.id = id, .reg = static_cast<register_id>(index), .kind = value_kind::phi,
            .inputs = {first.values[index], second.values[index]}});
        merged.values[index] = id;
    }
    return merged;
}

const std::vector<ssa_value>& register_ssa_builder::values() const noexcept { return values_; }

} // namespace dosrecomp::ir

