/** @file constant_propagation.hpp @brief SSA constant propagation over definitions and phi nodes. */
#pragma once

#include "dosrecomp/ir/register_ssa.hpp"

#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace dosrecomp::optimizer {

struct propagation_error { std::string message; };

/** Computes known 16-bit constants without mutating source SSA values. */
class constant_propagation final {
public:
    [[nodiscard]] static std::expected<std::vector<std::optional<std::uint16_t>>, propagation_error>
    analyze(const std::vector<ir::ssa_value>& values);
};

} // namespace dosrecomp::optimizer

