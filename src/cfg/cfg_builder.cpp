#include "dosrecomp/cfg/cfg_builder.hpp"

#include <algorithm>
#include <set>

namespace dosrecomp::cfg {
namespace {
[[nodiscard]] std::expected<std::size_t, cfg_error> target_for(
    const decoder::instruction& instruction, std::size_t code_size) {
    const auto next = static_cast<std::int64_t>(instruction.offset) + instruction.size;
    const auto target = next + instruction.relative_target;
    if (target < 0 || target >= static_cast<std::int64_t>(code_size)) {
        return std::unexpected(cfg_error{"branch target lies outside the code image"});
    }
    return static_cast<std::size_t>(target);
}

[[nodiscard]] bool ends_block(decoder::instruction_kind kind) {
    return kind == decoder::instruction_kind::call || kind == decoder::instruction_kind::jump ||
           kind == decoder::instruction_kind::conditional_jump || kind == decoder::instruction_kind::loop ||
           kind == decoder::instruction_kind::return_;
}
} // namespace

std::expected<control_flow_graph, cfg_error>
cfg_builder::build(const std::vector<std::byte>& code, std::size_t entry_offset) {
    if (entry_offset >= code.size()) return std::unexpected(cfg_error{"entry point lies outside the code image"});
    std::set<std::size_t> pending{entry_offset};
    std::set<std::size_t> seen;
    control_flow_graph graph;
    while (!pending.empty()) {
        const auto start = *pending.begin();
        pending.erase(pending.begin());
        if (seen.contains(start)) continue;
        basic_block block{.start = start, .end = start, .successors = {}};
        std::size_t position = start;
        while (true) {
            const auto decoded = decoder::instruction_decoder::decode_at(code, position);
            if (!decoded) return std::unexpected(cfg_error{decoded.error().message});
            const auto& instruction = *decoded;
            const std::size_t next = position + instruction.size;
            block.end = next;
            if (!ends_block(instruction.kind)) {
                if (next == code.size()) break;
                position = next;
                continue;
            }
            const bool has_direct_target = instruction.kind != decoder::instruction_kind::return_;
            if (has_direct_target) {
                const auto target = target_for(instruction, code.size());
                if (!target) return std::unexpected(target.error());
                block.successors.push_back(*target);
                pending.insert(*target);
            }
            const bool falls_through = instruction.kind == decoder::instruction_kind::call ||
                instruction.kind == decoder::instruction_kind::conditional_jump || instruction.kind == decoder::instruction_kind::loop;
            if (falls_through && next < code.size()) {
                block.successors.push_back(next);
                pending.insert(next);
            }
            break;
        }
        seen.insert(start);
        graph.blocks.push_back(std::move(block));
    }
    std::ranges::sort(graph.blocks, {}, &basic_block::start);
    return graph;
}

} // namespace dosrecomp::cfg

