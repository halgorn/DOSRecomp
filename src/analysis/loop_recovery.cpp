#include "dosrecomp/analysis/loop_recovery.hpp"

namespace dosrecomp::analysis {
std::expected<std::vector<recovered_loop>, loop_error>
loop_recovery::recover(const std::vector<std::byte>& code, const cfg::control_flow_graph& graph) {
    std::vector<recovered_loop> loops;
    for (const auto& block : graph.blocks) {
        std::size_t position = block.start;
        decoder::instruction last{};
        while (position < block.end) {
            const auto decoded = decoder::instruction_decoder::decode_at(code, position);
            if (!decoded || position + decoded->size > block.end) return std::unexpected(loop_error{"CFG block has invalid instruction boundaries"});
            last = *decoded;
            position += decoded->size;
        }
        const bool branches = last.kind == decoder::instruction_kind::jump ||
            last.kind == decoder::instruction_kind::conditional_jump || last.kind == decoder::instruction_kind::loop;
        if (!branches) continue;
        for (const auto target : block.successors) {
            if (target <= block.start) loops.push_back({.header = target, .latch = block.start});
        }
    }
    return loops;
}

} // namespace dosrecomp::analysis

