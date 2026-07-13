/** @file loop_recovery.hpp @brief Recovery of proven direct control-flow back edges. */
#pragma once

#include "dosrecomp/cfg/cfg_builder.hpp"

#include <cstddef>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::analysis {

/** A direct backward edge representing a CFG loop latch and header. */
struct recovered_loop { std::size_t header{}; std::size_t latch{}; };
struct loop_error { std::string message; };

/** Identifies direct JMP/Jcc/LOOP back edges without inferring source constructs. */
class loop_recovery final {
public:
    [[nodiscard]] static std::expected<std::vector<recovered_loop>, loop_error>
    recover(const std::vector<std::byte>& code, const cfg::control_flow_graph& graph);
};

} // namespace dosrecomp::analysis

