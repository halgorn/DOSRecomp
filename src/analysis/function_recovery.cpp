#include "dosrecomp/analysis/function_recovery.hpp"

#include <map>

namespace dosrecomp::analysis {
std::expected<std::vector<recovered_function>, recovery_error>
function_recovery::recover(const std::vector<std::byte>& code, const cfg::control_flow_graph& graph, std::size_t entry_offset) {
    std::map<std::size_t, std::vector<std::size_t>> functions;
    functions.emplace(entry_offset, std::vector<std::size_t>{});
    for (const auto& block : graph.blocks) {
        std::size_t position = block.start;
        decoder::instruction last{};
        while (position < block.end) {
            const auto decoded = decoder::instruction_decoder::decode_at(code, position);
            if (!decoded || position + decoded->size > block.end) return std::unexpected(recovery_error{"CFG block has invalid instruction boundaries"});
            last = *decoded;
            position += decoded->size;
        }
        if (position != block.end) return std::unexpected(recovery_error{"CFG block end is not an instruction boundary"});
        if (last.kind != decoder::instruction_kind::call) continue;
        const auto target = static_cast<std::int64_t>(block.end) + last.relative_target;
        if (target < 0 || target >= static_cast<std::int64_t>(code.size())) return std::unexpected(recovery_error{"direct call target is outside code"});
        functions[static_cast<std::size_t>(target)].push_back(last.offset);
    }
    std::vector<recovered_function> result;
    result.reserve(functions.size());
    for (auto& [entry, call_sites] : functions) result.push_back({entry, std::move(call_sites)});
    return result;
}

} // namespace dosrecomp::analysis

