/** @file function_recovery.hpp @brief Conservative recovery of direct-call function entries. */
#pragma once

#include "dosrecomp/cfg/cfg_builder.hpp"

#include <cstddef>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::analysis {

/** A function entry backed by one or more direct call sites. */
struct recovered_function {
    std::size_t entry{};
    std::vector<std::size_t> call_sites;
};

/** A decoding inconsistency detected during recovery. */
struct recovery_error { std::string message; };

/** Finds only proven direct-call targets; jumps never create functions. */
class function_recovery final {
public:
    [[nodiscard]] static std::expected<std::vector<recovered_function>, recovery_error>
    recover(const std::vector<std::byte>& code, const cfg::control_flow_graph& graph, std::size_t entry_offset);
};

} // namespace dosrecomp::analysis

