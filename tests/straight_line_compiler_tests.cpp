#include "dosrecomp/compiler/straight_line_compiler.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

namespace { std::byte b(unsigned value) { return static_cast<std::byte>(value); } }

int main() {
    const auto loaded = dosrecomp::loader::binary_loader::load_bytes({b(0xb8), b(7), b(0x4c), b(0xcd), b(0x21)});
    if (!loaded) return EXIT_FAILURE;
    const auto elf = dosrecomp::compiler::straight_line_compiler::compile(*loaded);
    const auto exit_code = dosrecomp::compiler::straight_line_compiler::extract_exit_code(*loaded);
    const auto llvm = dosrecomp::compiler::straight_line_compiler::emit_llvm(*loaded);
    if (!elf || !exit_code || !llvm || llvm->find("ret i32 7") == std::string::npos || *exit_code != 7) {
        std::cerr << "failed straight-line exit recompilation\n";
        return EXIT_FAILURE;
    }
    const auto arithmetic_exit = dosrecomp::loader::binary_loader::load_bytes({b(0xb8), b(2), b(0x4c), b(0x05), b(3), b(0), b(0xcd), b(0x21)});
    const auto arith_elf = arithmetic_exit ? dosrecomp::compiler::straight_line_compiler::compile(*arithmetic_exit) : std::expected<std::vector<std::byte>, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"arithmetic load failed"})};
    const auto arith_code = arithmetic_exit ? dosrecomp::compiler::straight_line_compiler::extract_exit_code(*arithmetic_exit) : std::expected<std::uint8_t, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"arithmetic code failed"})};
    if (!arith_elf || !arith_code || *arith_code != 5) {
        std::cerr << "failed straight-line arithmetic exit\n";
        return EXIT_FAILURE;
    }
    const auto reg_reg_exit = dosrecomp::loader::binary_loader::load_bytes({b(0xb8), b(3), b(0x00), b(0xbb), b(2), b(0x00), b(0x01), b(0xd8), b(0xb8), b(5), b(0x4c), b(0xcd), b(0x21)});
    const auto reg_reg_elf = reg_reg_exit ? dosrecomp::compiler::straight_line_compiler::compile(*reg_reg_exit) : std::expected<std::vector<std::byte>, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"reg-reg load failed"})};
    const auto reg_reg_code = reg_reg_exit ? dosrecomp::compiler::straight_line_compiler::extract_exit_code(*reg_reg_exit) : std::expected<std::uint8_t, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"reg-reg code failed"})};
    if (!reg_reg_elf || !reg_reg_code || *reg_reg_code != static_cast<std::uint8_t>(0x4c05U)) {
        std::cerr << "failed straight-line reg-reg exit\n";
        return EXIT_FAILURE;
    }
    const auto taken_branch = dosrecomp::loader::binary_loader::load_bytes({b(0xb8), b(7), b(0x4c), b(0x74), b(0x02), b(0xb8), b(9), b(0x4c), b(0xcd), b(0x21)});
    const auto taken_elf = taken_branch ? dosrecomp::compiler::straight_line_compiler::compile(*taken_branch) : std::expected<std::vector<std::byte>, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"taken branch load failed"})};
    const auto taken_code = taken_branch ? dosrecomp::compiler::straight_line_compiler::extract_exit_code(*taken_branch) : std::expected<std::uint8_t, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"taken branch code failed"})};
    if (!taken_elf || !taken_code || *taken_code != 9) {
        std::cerr << "failed to evaluate constant conditional branch (JE not taken)\n";
        return EXIT_FAILURE;
    }
    const auto skipped_branch = dosrecomp::loader::binary_loader::load_bytes({b(0xb8), b(5), b(0x00), b(0x39), b(0xc0), b(0x74), b(0x02), b(0xb8), b(9), b(0x4c), b(0xb8), b(5), b(0x4c), b(0xcd), b(0x21)});
    const auto skipped_code = skipped_branch ? dosrecomp::compiler::straight_line_compiler::extract_exit_code(*skipped_branch) : std::expected<std::uint8_t, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"skipped branch code failed"})};
    if (!skipped_code || *skipped_code != 5) {
        std::cerr << "failed to evaluate constant conditional branch (JE taken via CMP AX,AX)\n";
        return EXIT_FAILURE;
    }
    const auto byte_form = dosrecomp::loader::binary_loader::load_bytes({b(0xb4), b(0x4c), b(0xb0), b(9), b(0xcd), b(0x21)});
    const auto byte_elf = byte_form ? dosrecomp::compiler::straight_line_compiler::compile(*byte_form) : std::expected<std::vector<std::byte>, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"byte load failed"})};
    const auto byte_code = byte_form ? dosrecomp::compiler::straight_line_compiler::extract_exit_code(*byte_form) : std::expected<std::uint8_t, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"byte code failed"})};
    if (!byte_elf || !byte_code || *byte_code != 9) {
        std::cerr << "failed straight-line byte-form exit (MOV AH,4Ch MOV AL,9)\n";
        return EXIT_FAILURE;
    }
    const auto char_program = dosrecomp::loader::binary_loader::load_bytes({b(0xb2), b('A'), b(0xb4), b(0x02), b(0xcd), b(0x21), b(0xb4), b(0x4c), b(0xb0), b(7), b(0xcd), b(0x21)});
    const auto char_elf = char_program ? dosrecomp::compiler::straight_line_compiler::compile(*char_program) : std::expected<std::vector<std::byte>, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"char load failed"})};
    if (!char_elf) {
        std::cerr << "failed to compile INT 21h AH=02h write-char program\n";
        return EXIT_FAILURE;
    }
    const auto char_path = std::filesystem::temp_directory_path() / ("dosrecomp-char-" + std::to_string(getpid()));
    {
        std::ofstream output(char_path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(char_elf->data()), static_cast<std::streamsize>(char_elf->size()));
    }
    std::filesystem::permissions(char_path, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
    const auto out_path = char_path.string() + ".out";
    const auto err_path = char_path.string() + ".err";
    const auto cmd = char_path.string() + " > " + out_path + " 2> " + err_path;
    const auto char_status = std::system(cmd.c_str());
    std::ifstream stdout_file(out_path, std::ios::binary);
    std::ifstream stderr_file(err_path, std::ios::binary);
    std::string stdout_text{std::istreambuf_iterator<char>{stdout_file}, {}};
    std::string stderr_text{std::istreambuf_iterator<char>{stderr_file}, {}};
    std::filesystem::remove(char_path);
    std::filesystem::remove(out_path);
    std::filesystem::remove(err_path);
    if (char_status == -1 || !WIFEXITED(char_status) || WEXITSTATUS(char_status) != 7 ||
        stderr_text != "A" || !stdout_text.empty()) {
        std::cerr << "INT 21h AH=02h did not write to stderr as expected\n";
        return EXIT_FAILURE;
    }
    const auto video_program = dosrecomp::loader::binary_loader::load_bytes({b(0xb0), b('H'), b(0xb4), b(0x0e), b(0xcd), b(0x10), b(0xb4), b(0x4c), b(0xb0), b(7), b(0xcd), b(0x21)});
    const auto video_elf = video_program ? dosrecomp::compiler::straight_line_compiler::compile(*video_program) : std::expected<std::vector<std::byte>, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"video load failed"})};
    if (!video_elf) {
        std::cerr << "failed to compile INT 10h AH=0Eh program\n";
        return EXIT_FAILURE;
    }
    const auto video_path = std::filesystem::temp_directory_path() / ("dosrecomp-video-" + std::to_string(getpid()));
    {
        std::ofstream output(video_path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(video_elf->data()), static_cast<std::streamsize>(video_elf->size()));
    }
    std::filesystem::permissions(video_path, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
    const auto video_status = std::system(video_path.string().c_str());
    std::filesystem::remove(video_path);
    if (video_status == -1 || !WIFEXITED(video_status) || WEXITSTATUS(video_status) != 7) {
        std::cerr << "INT 10h AH=0Eh program did not exit 7\n";
        return EXIT_FAILURE;
    }
    const auto non_exit = dosrecomp::loader::binary_loader::load_bytes({b(0x90)});
    const auto non_exit_elf = non_exit ? dosrecomp::compiler::straight_line_compiler::compile(*non_exit) : std::expected<std::vector<std::byte>, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"non-exit load failed"})};
    if (non_exit_elf) {
        std::cerr << "failed to reject non-exit program\n";
        return EXIT_FAILURE;
    }
    const auto executable = std::filesystem::temp_directory_path() / ("dosrecomp-sl-" + std::to_string(getpid()));
    {
        std::ofstream output(executable, std::ios::binary);
        output.write(reinterpret_cast<const char*>(reg_reg_elf->data()), static_cast<std::streamsize>(reg_reg_elf->size()));
    }
    std::filesystem::permissions(executable, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
    const auto status = std::system(executable.string().c_str());
    std::filesystem::remove(executable);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 5) {
        std::cerr << "native straight-line recompilation did not exit with 5\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}