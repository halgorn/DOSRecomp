#include "dosrecomp/optimizer/constant_propagation.hpp"

namespace dosrecomp::optimizer {
std::expected<std::vector<std::optional<std::uint16_t>>, propagation_error>
constant_propagation::analyze(const std::vector<ir::ssa_value>& values) {
    std::vector<std::optional<std::uint16_t>> result(values.size());
    for (const auto& value : values) {
        if (value.id >= values.size()) return std::unexpected(propagation_error{"SSA value ID is outside the value table"});
        result[value.id] = value.constant;
        if (value.kind != ir::value_kind::phi || value.inputs.empty()) continue;
        const auto first = value.inputs.front();
        if (first >= value.id) return std::unexpected(propagation_error{"phi input is not a prior SSA value"});
        if (!result[first]) continue;
        for (const auto input : value.inputs) {
            if (input >= value.id) return std::unexpected(propagation_error{"phi input is not a prior SSA value"});
            if (!result[input] || result[input] != result[first]) {
                result[value.id] = std::nullopt;
                break;
            }
            result[value.id] = result[first];
        }
    }
    return result;
}

} // namespace dosrecomp::optimizer
