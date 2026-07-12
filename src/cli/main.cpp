#include "dosrecomp/loader/binary_loader.hpp"
#include "dosrecomp/cfg/cfg_builder.hpp"
#include "dosrecomp/ir/control_flow_ir.hpp"

#include <iostream>
#include <string_view>

namespace {
void print_usage() {
    std::cerr << "Usage: dosrecomp <input.com|input.exe> [--verbose|--emit-cfg|--emit-ir]\n";
}
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        print_usage();
        return 2;
    }
    const auto option = argc == 3 ? std::string_view(argv[2]) : std::string_view{};
    const bool verbose = option == "--verbose";
    const bool emit_cfg = option == "--emit-cfg";
    const bool emit_ir = option == "--emit-ir";
    if (argc == 3 && !verbose && !emit_cfg && !emit_ir) {
        print_usage();
        return 2;
    }
    const auto result = dosrecomp::loader::binary_loader::load_file(argv[1]);
    if (!result) {
        std::cerr << "dosrecomp: " << result.error().message << '\n';
        return 1;
    }
    if (verbose) {
        const auto& image = *result;
        std::cout << "format: " << (image.format == dosrecomp::loader::executable_format::com ? "COM" : "MZ")
                  << "\nload module bytes: " << image.bytes.size()
                  << "\nrelocations: " << image.relocations.size() << '\n';
    }
    if (emit_cfg || emit_ir) {
        const auto graph = dosrecomp::cfg::cfg_builder::build(result->bytes, result->entry_offset());
        if (!graph) {
            std::cerr << "dosrecomp: cannot build CFG: " << graph.error().message << '\n';
            return 1;
        }
        if (emit_ir) {
            const auto ir = dosrecomp::ir::control_flow_lowerer::lower(*graph);
            if (!ir) {
                std::cerr << "dosrecomp: cannot lower IR: " << ir.error().message << '\n';
                return 1;
            }
            for (const auto& block : ir->blocks) {
                std::cout << "block " << block.id << " @0x" << std::hex << block.source_start << std::dec << ": ";
                if (block.terminator == dosrecomp::ir::terminator_kind::stop) std::cout << "stop";
                if (block.terminator == dosrecomp::ir::terminator_kind::jump) std::cout << "jump";
                if (block.terminator == dosrecomp::ir::terminator_kind::branch) std::cout << "branch";
                for (const auto successor : block.successors) std::cout << " " << successor;
                std::cout << '\n';
            }
            return 0;
        }
        std::cout << "digraph cfg {\n";
        for (const auto& block : graph->blocks) {
            std::cout << "  b" << block.start << " [label=\"0x" << std::hex << block.start << "\"];\n";
            for (const auto successor : block.successors) std::cout << "  b" << block.start << " -> b" << successor << ";\n";
        }
        std::cout << "}\n";
        return 0;
    }
    std::cout << "Loaded successfully. Recompilation pipeline is not implemented yet.\n";
}
