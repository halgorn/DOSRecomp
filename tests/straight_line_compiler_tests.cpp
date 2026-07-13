#include "dosrecomp/compiler/straight_line_compiler.hpp"
#include "dosrecomp/compiler/branching_compiler.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>

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
    const auto integrated = dosrecomp::loader::binary_loader::load_bytes({
        b(0xb8), b(5), b(0x00),          // mov ax, 5
        b(0xb4), b(0x00),                // mov ah, 0
        b(0x3d), b(5), b(0x00),          // cmp ax, 5
        b(0x75), b(7),                   // jne +7  (skip hi block to offset 17)
        b(0xba), b(0x21), b(0x01),       // mov dx, 0x0121 = "hi"
        b(0xb8), b(0x00), b(0x09),       // mov ax, 0x0900
        b(0xcd), b(0x21),               // int 21h print "hi"
        b(0xeb), b(8),                   // jmp +8  (skip bye block to offset 28)
        b(0xba), b(0x24), b(0x01),       // mov dx, 0x0124 = "bye"
        b(0xb8), b(0x00), b(0x09),       // mov ax, 0x0900
        b(0xcd), b(0x21),               // int 21h print "bye"
        b(0xb8), b(8), b(0x4c),         // mov ax, 0x4c08
        b(0xcd), b(0x21),               // int 21h exit
        b('h'), b('i'), b('$'),          // "hi$"  (offset 33)
        b('b'), b('y'), b('e'), b('$')   // "bye$" (offset 36)
    });
    const auto integrated_elf = integrated ? dosrecomp::compiler::straight_line_compiler::compile(*integrated) : std::expected<std::vector<std::byte>, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"integrated load failed"})};
    if (!integrated_elf) {
        std::cerr << "failed to compile integrated branch+console program\n";
        return EXIT_FAILURE;
    }
    const auto integrated_path = std::filesystem::temp_directory_path() / ("dosrecomp-int-" + std::to_string(getpid()));
    {
        std::ofstream output(integrated_path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(integrated_elf->data()), static_cast<std::streamsize>(integrated_elf->size()));
    }
    std::filesystem::permissions(integrated_path, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
    const auto integrated_out = integrated_path.string() + ".captured";
    const auto integrated_cmd = integrated_path.string() + " > " + integrated_out;
    const auto integrated_status = std::system(integrated_cmd.c_str());
    std::ifstream captured(integrated_out, std::ios::binary);
    std::string integrated_text{std::istreambuf_iterator<char>{captured}, {}};
    std::filesystem::remove(integrated_path);
    std::filesystem::remove(integrated_out);
    if (integrated_status == -1 || !WIFEXITED(integrated_status) || WEXITSTATUS(integrated_status) != 8 ||
        integrated_text != "hi") {
        std::cerr << "integrated branch program did not exit 8 or did not print 'hi'\n";
        return EXIT_FAILURE;
    }
    const auto filewrite = std::vector<std::byte>{
        b(0xba), b(0x1d), b(0x01),
        b(0xb8), b(0x01), b(0x3d),
        b(0xcd), b(0x21),
        b(0x89), b(0xc3),
        b(0xba), b(0x25), b(0x01),
        b(0xb9), b(0x06), b(0x00),
        b(0xb4), b(0x40),
        b(0xcd), b(0x21),
        b(0xb4), b(0x3e),
        b(0xcd), b(0x21),
        b(0xb8), b(0x00), b(0x4c),
        b(0xcd), b(0x21),
        b('o'), b('u'), b('t'), b('.'), b('t'), b('x'), b('t'), b(0),
        b('h'), b('e'), b('l'), b('l'), b('o'), b('\n'),
    };
    const auto filewrite_image = dosrecomp::loader::binary_loader::load_bytes(filewrite);
    const auto filewrite_elf = filewrite_image ? dosrecomp::compiler::straight_line_compiler::compile(*filewrite_image) : std::expected<std::vector<std::byte>, dosrecomp::compiler::straight_line_compile_error>{std::unexpected(dosrecomp::compiler::straight_line_compile_error{"filewrite load failed"})};
    if (!filewrite_elf) {
        std::cerr << "failed to compile file IO program\n";
        return EXIT_FAILURE;
    }
    const auto fw_path = std::filesystem::temp_directory_path() / ("dosrecomp-fw-" + std::to_string(getpid()));
    {
        std::ofstream output(fw_path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(filewrite_elf->data()), static_cast<std::streamsize>(filewrite_elf->size()));
    }
    std::filesystem::permissions(fw_path, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
    if (!filewrite_elf) {
        std::cerr << "failed to compile file IO program\n";
        return EXIT_FAILURE;
    }
    (void)fw_path;
    std::filesystem::remove(fw_path);
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
    const auto runtime_branch = std::vector<std::byte>{
        b(0xb4), b(0x01),
        b(0xcd), b(0x21),
        b(0x3c), b(0x79),
        b(0x74), b(0x05),
        b(0xb8), b(0x01), b(0x4c),
        b(0xeb), b(0x03),
        b(0xb8), b(0x02), b(0x4c),
        b(0xcd), b(0x21),
    };
    const auto rb_image = dosrecomp::loader::binary_loader::load_bytes(runtime_branch);
    if (!rb_image) {
        std::cerr << "failed to load runtime branch program\n";
        return EXIT_FAILURE;
    }
    const auto rb_elf = dosrecomp::compiler::branching_compiler::compile(*rb_image);
    if (!rb_elf) {
        std::cerr << "failed to compile runtime branch program: " << rb_elf.error().message << "\n";
        return EXIT_FAILURE;
    }
    const auto rb_path = std::filesystem::temp_directory_path() / ("dosrecomp-rb-" + std::to_string(getpid()));
    {
        std::ofstream output(rb_path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(rb_elf->data()), static_cast<std::streamsize>(rb_elf->size()));
    }
    std::filesystem::permissions(rb_path, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
    auto run_with_stdin = [&](const char* input) {
        const auto pid = fork();
        if (pid == 0) {
            const std::string cmd = std::string{"echo "} + input + " | " + rb_path.string();
            execl("/bin/sh", "sh", "-c", cmd.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }
        int st = 0;
        waitpid(pid, &st, 0);
        return st;
    };
    const auto y_status = run_with_stdin("y");
    const auto n_status = run_with_stdin("n");
    std::filesystem::remove(rb_path);
    if (!WIFEXITED(y_status) || WEXITSTATUS(y_status) != 2 ||
        !WIFEXITED(n_status) || WEXITSTATUS(n_status) != 1) {
        std::cerr << "runtime branch program did not exit 2/1 for y/n\n";
        return EXIT_FAILURE;
    }
    const auto int16_program = std::vector<std::byte>{
        b(0xb4), b(0x00),
        b(0xcd), b(0x16),
        b(0x3c), b(0x79),
        b(0x74), b(0x05),
        b(0xb8), b(0x01), b(0x4c),
        b(0xeb), b(0x03),
        b(0xb8), b(0x02), b(0x4c),
        b(0xcd), b(0x21),
    };
    const auto int16_image = dosrecomp::loader::binary_loader::load_bytes(int16_program);
    if (!int16_image) {
        std::cerr << "failed to load INT 16h program\n";
        return EXIT_FAILURE;
    }
    const auto int16_elf = dosrecomp::compiler::branching_compiler::compile(*int16_image);
    if (!int16_elf) {
        std::cerr << "failed to compile INT 16h program: " << int16_elf.error().message << "\n";
        return EXIT_FAILURE;
    }
    const auto i16_path = std::filesystem::temp_directory_path() / ("dosrecomp-i16-" + std::to_string(getpid()));
    {
        std::ofstream output(i16_path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(int16_elf->data()), static_cast<std::streamsize>(int16_elf->size()));
    }
    std::filesystem::permissions(i16_path, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
    auto run_with_stdin_i16 = [&](const char* input) {
        const auto pid = fork();
        if (pid == 0) {
            const std::string cmd = std::string{"echo "} + input + " | " + i16_path.string();
            execl("/bin/sh", "sh", "-c", cmd.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }
        int st = 0;
        waitpid(pid, &st, 0);
        return st;
    };
    const auto i16_y = run_with_stdin_i16("y");
    const auto i16_n = run_with_stdin_i16("n");
    std::filesystem::remove(i16_path);
    if (!WIFEXITED(i16_y) || WEXITSTATUS(i16_y) != 2 ||
        !WIFEXITED(i16_n) || WEXITSTATUS(i16_n) != 1) {
        std::cerr << "INT 16h program did not exit 2/1 for y/n\n";
        return EXIT_FAILURE;
    }
    const auto callret_program = std::vector<std::byte>{
        b(0xb4), b(0x07),
        b(0xe8), b(0x04), b(0x00),
        b(0xb4), b(0x4c),
        b(0xcd), b(0x21),
        b(0x88), b(0xe0),
        b(0xc3),
    };
    const auto callret_image = dosrecomp::loader::binary_loader::load_bytes(callret_program);
    if (!callret_image) { std::cerr << "failed to load CALL/RET program\n"; return EXIT_FAILURE; }
    const auto callret_elf = dosrecomp::compiler::branching_compiler::compile(*callret_image);
    if (!callret_elf) { std::cerr << "failed to compile CALL/RET program: " << callret_elf.error().message << "\n"; return EXIT_FAILURE; }
    const auto cr_path = std::filesystem::temp_directory_path() / ("dosrecomp-cr-" + std::to_string(getpid()));
    { std::ofstream output(cr_path, std::ios::binary); output.write(reinterpret_cast<const char*>(callret_elf->data()), static_cast<std::streamsize>(callret_elf->size())); }
    std::filesystem::permissions(cr_path, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
    const auto cr_pid = fork();
    if (cr_pid == 0) { execl(cr_path.string().c_str(), cr_path.string().c_str(), static_cast<char*>(nullptr)); _exit(127); }
    int cr_status = 0; waitpid(cr_pid, &cr_status, 0);
    std::filesystem::remove(cr_path);
    if (!WIFEXITED(cr_status) || WEXITSTATUS(cr_status) != 7) {
        std::cerr << "CALL/RET program did not exit 7\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}