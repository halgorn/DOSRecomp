#include "dosrecomp/optimizer/dead_value_elimination.hpp"

namespace dosrecomp::optimizer {
std::expected<std::vector<bool>, liveness_error>
dead_value_elimination::analyze(const std::vector<ir::ssa_value>& values, const std::vector<std::size_t>& roots) {
    std::vector<bool> live(values.size(), false);
    std::vector<std::size_t> pending = roots;
    while (!pending.empty()) {
        const auto value_id = pending.back();
        pending.pop_back();
        if (value_id >= values.size()) return std::unexpected(liveness_error{"SSA liveness root is outside the value table"});
        if (live[value_id]) continue;
        const auto& value = values[value_id];
        if (value.id != value_id) return std::unexpected(liveness_error{"SSA value IDs are not dense"});
        live[value_id] = true;
        for (const auto input : value.inputs) {
            if (input >= values.size()) return std::unexpected(liveness_error{"SSA input is outside the value table"});
            pending.push_back(input);
        }
    }
    return live;
}

} // namespace dosrecomp::optimizer

