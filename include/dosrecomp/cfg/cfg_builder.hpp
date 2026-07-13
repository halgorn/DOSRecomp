/** @file cfg_builder.hpp @brief Recovery of reachable basic blocks from decoded 8086 code. */
#pragma once

#include "dosrecomp/decoder/instruction_decoder.hpp"

#include <cstddef>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::cfg {

/** A basic block bounded by instruction offsets, with direct successor offsets. */
struct basic_block {
    std::size_t start{};
    std::size_t end{};
    std::vector<std::size_t> successors;
    decoder::branch_condition condition{decoder::branch_condition::always};
};

/** Reachable direct-control-flow graph. */
struct control_flow_graph { std::vector<basic_block> blocks; };

/** A decoder failure or invalid target encountered while forming the graph. */
struct cfg_error { std::string message; };

/**
 * Builds a graph from a code byte span and an entry offset relative to that span.
 * Indirect calls/jumps remain unsupported until operand decoding is available.
 */
class cfg_builder final {
public:
    [[nodiscard]] static std::expected<control_flow_graph, cfg_error>
    build(const std::vector<std::byte>& code, std::size_t entry_offset);
};

} // namespace dosrecomp::cfg
