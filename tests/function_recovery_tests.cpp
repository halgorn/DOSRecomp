#include "dosrecomp/analysis/function_recovery.hpp"

#include <cstdlib>
#include <iostream>

namespace { std::byte b(unsigned value) { return static_cast<std::byte>(value); } }

int main() {
    const std::vector<std::byte> code{b(0xe8), b(0x01), b(0x00), b(0xc3), b(0x90), b(0xc3)};
    const auto graph = dosrecomp::cfg::cfg_builder::build(code, 0);
    const auto functions = graph ? dosrecomp::analysis::function_recovery::recover(code, *graph, 0)
                                 : std::expected<std::vector<dosrecomp::analysis::recovered_function>, dosrecomp::analysis::recovery_error>{std::unexpected(dosrecomp::analysis::recovery_error{"CFG failed"})};
    if (!functions || functions->size() != 2 || (*functions)[0].entry != 0 || (*functions)[1].entry != 4 ||
        (*functions)[1].call_sites != std::vector<std::size_t>{0}) {
        std::cerr << "failed direct-call function recovery\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

