/** @file dead_value_elimination.hpp @brief Liveness analysis for removing unused SSA values. */
#pragma once

#include "dosrecomp/ir/register_ssa.hpp"

#include <cstddef>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::optimizer {

struct liveness_error { std::string message; };

/** Marks roots and all SSA dependencies they require as live. */
class dead_value_elimination final {
public:
    [[nodiscard]] static std::expected<std::vector<bool>, liveness_error>
    analyze(const std::vector<ir::ssa_value>& values, const std::vector<std::size_t>& roots);
};

} // namespace dosrecomp::optimizer

