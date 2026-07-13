#include "dosrecomp/ir/control_flow_ir.hpp"

#include <unordered_map>

namespace dosrecomp::ir {
std::expected<control_flow_program, ir_error>
control_flow_lowerer::lower(const cfg::control_flow_graph& graph) {
    std::unordered_map<std::size_t, std::size_t> ids;
    ids.reserve(graph.blocks.size());
    for (std::size_t index = 0; index < graph.blocks.size(); ++index) {
        const auto inserted = ids.emplace(graph.blocks[index].start, index).second;
        if (!inserted) return std::unexpected(ir_error{"CFG contains duplicate block starts"});
    }
    control_flow_program program;
    program.blocks.reserve(graph.blocks.size());
    for (std::size_t index = 0; index < graph.blocks.size(); ++index) {
        const auto& source = graph.blocks[index];
        ir_block block{.id = index, .source_start = source.start, .terminator = terminator_kind::stop, .successors = {}, .condition = source.condition};
        for (const auto target : source.successors) {
            const auto found = ids.find(target);
            if (found == ids.end()) return std::unexpected(ir_error{"CFG successor does not start a block"});
            block.successors.push_back(found->second);
        }
        if (block.successors.size() == 1) block.terminator = terminator_kind::jump;
        if (block.successors.size() == 2) block.terminator = terminator_kind::branch;
        if (block.successors.size() > 2) return std::unexpected(ir_error{"CFG block has more than two direct successors"});
        program.blocks.push_back(std::move(block));
    }
    return program;
}

} // namespace dosrecomp::ir
