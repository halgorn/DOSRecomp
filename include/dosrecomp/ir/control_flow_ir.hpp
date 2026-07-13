/** @file control_flow_ir.hpp @brief Target-independent control-flow IR lowered from a CFG. */
#pragma once

#include "dosrecomp/cfg/cfg_builder.hpp"

#include <cstddef>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::ir {

/** The control transfer ending a lowered IR block. */
enum class terminator_kind { stop, jump, branch };

/** Immutable IR block identity and its direct successors. */
struct ir_block {
    std::size_t id{};
    std::size_t source_start{};
    terminator_kind terminator{};
    std::vector<std::size_t> successors;
    decoder::branch_condition condition{decoder::branch_condition::always};
};

/** A stable, target-independent control-flow program representation. */
struct control_flow_program { std::vector<ir_block> blocks; };

/** An invalid CFG that cannot be represented as IR. */
struct ir_error { std::string message; };

/** Lowers validated CFG blocks into immutable IDs suitable for SSA expansion. */
class control_flow_lowerer final {
public:
    [[nodiscard]] static std::expected<control_flow_program, ir_error>
    lower(const cfg::control_flow_graph& graph);
};

} // namespace dosrecomp::ir
