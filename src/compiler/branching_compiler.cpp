#include "dosrecomp/compiler/branching_compiler.hpp"

#include "dosrecomp/compiler/straight_line_compiler.hpp"

namespace dosrecomp::compiler {

std::expected<std::uint8_t, branching_compile_error>
branching_compiler::extract_exit_code(const loader::program_image& image) {
    const auto result = straight_line_compiler::extract_exit_code(image);
    if (!result) return std::unexpected(branching_compile_error{result.error().message});
    return *result;
}

std::expected<std::vector<std::byte>, branching_compile_error>
branching_compiler::compile(const loader::program_image& image) {
    const auto result = straight_line_compiler::compile(image);
    if (!result) return std::unexpected(branching_compile_error{result.error().message});
    return *result;
}

std::expected<std::string, branching_compile_error>
branching_compiler::emit_llvm(const loader::program_image& image) {
    const auto result = straight_line_compiler::emit_llvm(image);
    if (!result) return std::unexpected(branching_compile_error{result.error().message});
    return *result;
}

} // namespace dosrecomp::compiler