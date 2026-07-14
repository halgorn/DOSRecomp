#include "dosrecomp/loader/binary_loader.hpp"
#include "dosrecomp/cfg/cfg_builder.hpp"
#include "dosrecomp/compiler/branching_compiler.hpp"
#include "dosrecomp/compiler/cpp_backend.hpp"
#include "dosrecomp/compiler/straight_line_compiler.hpp"
#include "dosrecomp/ir/control_flow_ir.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {
void print_usage() {
    std::cerr << "Usage: dosrecomp <input.com|input.exe> [-o output|--verbose|--emit-cfg|--emit-dot|--emit-ir|--emit-cpp|--emit-llvm|--keep-cpp]\n";
}
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 5) {
        print_usage();
        return 2;
    }
    const auto option = argc == 3 ? std::string_view(argv[2])
                          : argc == 5 ? std::string_view(argv[4])
                          : std::string_view{};
    const bool verbose = option == "--verbose";
    const bool emit_cfg = option == "--emit-cfg";
    const bool emit_dot = option == "--emit-dot";
    const bool emit_ir = option == "--emit-ir";
    const bool emit_cpp = option == "--emit-cpp";
    const bool emit_llvm = option == "--emit-llvm";
    const bool keep_cpp = option == "--keep-cpp";
    const bool explicit_output = (argc == 4 || argc == 5) && std::string_view(argv[2]) == "-o";
    if ((argc == 3 && !verbose && !emit_cfg && !emit_dot && !emit_ir && !emit_cpp && !emit_llvm && !keep_cpp) || ((argc == 4 || argc == 5) && !explicit_output)) {
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
                  << "\nrelocations: " << image.relocations.size()
                  << "\nentry: " << std::hex << image.entry_point.segment << ":" << image.entry_point.offset << std::dec
                  << "\nstack: " << std::hex << image.initial_stack.segment << ":" << image.initial_stack.offset << std::dec << '\n';
        const auto graph = dosrecomp::cfg::cfg_builder::build(result->bytes, result->entry_offset());
        if (graph) std::cout << "blocks: " << graph->blocks.size() << '\n';
    }
    if (emit_cpp) {
        const auto cpp = dosrecomp::compiler::cpp_backend::emit(*result);
        if (!cpp) {
            std::cerr << "dosrecomp: cannot emit C++: " << cpp.error().message << '\n';
            return 1;
        }
        std::cout << *cpp;
        return 0;
    }
    if (emit_llvm) {
        const auto llvm = dosrecomp::compiler::straight_line_compiler::emit_llvm(*result);
        if (!llvm) {
            std::cerr << "dosrecomp: cannot emit LLVM IR: " << llvm.error().message << '\n';
            return 1;
        }
        std::cout << *llvm;
        return 0;
    }
    if (emit_cfg || emit_dot || emit_ir) {
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
    if (verbose) std::cout << "compiling...\n";
    auto output_path = std::filesystem::path(argv[1]);
    if (explicit_output) output_path = argv[3];
    else output_path.replace_extension();
    if (output_path.empty()) {
        std::cerr << "dosrecomp: cannot infer an output path\n";
        return 1;
    }
    auto executable = dosrecomp::compiler::branching_compiler::compile(*result);
    std::vector<std::byte> executable_bytes;
    bool got_executable = false;
    if (executable) { executable_bytes = std::move(*executable); got_executable = true; }
    else {
        const auto fallback = dosrecomp::compiler::straight_line_compiler::compile(*result);
        if (fallback) { executable_bytes = std::move(*fallback); got_executable = true; }
        else {
            std::cerr << "dosrecomp: cannot compile: " << executable.error().message << '\n';
            return 1;
        }
    }
    (void)got_executable;
    std::vector<std::byte>& bytes = executable_bytes;
    std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
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
    if (verbose) {
        std::cout << "compiled " << bytes.size() << " bytes\n";
    }
    std::cout << "Wrote " << output_path.string() << '\n';
    if (keep_cpp) {
        if (executable) {
            const auto cpp = dosrecomp::compiler::branching_compiler::emit_c_source(*result);
            if (cpp) {
                auto cpp_path = output_path;
                cpp_path.replace_extension(".cpp");
                std::ofstream cpp_out(cpp_path, std::ios::binary | std::ios::trunc);
                cpp_out.write(cpp->data(), static_cast<std::streamsize>(cpp->size()));
                if (cpp_out) std::cout << "Wrote " << cpp_path.string() << '\n';
            }
        } else {
            const auto cpp = dosrecomp::compiler::cpp_backend::emit(*result);
            if (cpp) {
                auto cpp_path = output_path;
                cpp_path.replace_extension(".cpp");
                std::ofstream cpp_out(cpp_path, std::ios::binary | std::ios::trunc);
                cpp_out.write(cpp->data(), static_cast<std::streamsize>(cpp->size()));
                if (cpp_out) std::cout << "Wrote " << cpp_path.string() << '\n';
            }
        }
    }
}
