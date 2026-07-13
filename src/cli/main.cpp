#include "dosrecomp/loader/binary_loader.hpp"
#include "dosrecomp/cfg/cfg_builder.hpp"
#include "dosrecomp/compiler/exit_program_compiler.hpp"
#include "dosrecomp/ir/control_flow_ir.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {
void print_usage() {
    std::cerr << "Usage: dosrecomp <input.com|input.exe> [-o output|--verbose|--emit-cfg|--emit-ir|--emit-cpp]\n";
}
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 4) {
        print_usage();
        return 2;
    }
    const auto option = argc == 3 ? std::string_view(argv[2]) : std::string_view{};
    const bool verbose = option == "--verbose";
    const bool emit_cfg = option == "--emit-cfg";
    const bool emit_ir = option == "--emit-ir";
    const bool emit_cpp = option == "--emit-cpp";
    const bool explicit_output = argc == 4 && std::string_view(argv[2]) == "-o";
    if ((argc == 3 && !verbose && !emit_cfg && !emit_ir && !emit_cpp) || (argc == 4 && !explicit_output)) {
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
    if (emit_cpp) {
        const auto exit_code = dosrecomp::compiler::exit_program_compiler::extract_exit_code(*result);
        if (!exit_code) {
            std::cerr << "dosrecomp: cannot emit C++: " << exit_code.error().message << '\n';
            return 1;
        }
        std::cout << "#include <cstdlib>\n\nint main() { return " << static_cast<unsigned>(*exit_code) << "; }\n";
        return 0;
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
    auto output_path = std::filesystem::path(argv[1]);
    if (explicit_output) output_path = argv[3];
    else output_path.replace_extension();
    if (output_path.empty()) {
        std::cerr << "dosrecomp: cannot infer an output path\n";
        return 1;
    }
    const auto executable = dosrecomp::compiler::exit_program_compiler::compile(*result);
    if (!executable) {
        std::cerr << "dosrecomp: cannot compile: " << executable.error().message << '\n';
        return 1;
    }
    std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(executable->data()), static_cast<std::streamsize>(executable->size()));
    if (!output) {
        std::cerr << "dosrecomp: cannot write '" << output_path.string() << "'\n";
        return 1;
    }
    output.close();
    std::error_code error;
    std::filesystem::permissions(output_path, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add, error);
    if (error) {
        std::cerr << "dosrecomp: cannot make output executable: " << error.message() << '\n';
        return 1;
    }
    std::cout << "Wrote " << output_path.string() << '\n';
}
