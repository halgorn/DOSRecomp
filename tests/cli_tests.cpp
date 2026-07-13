#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include <sys/wait.h>
#include <unistd.h>

namespace {
int run(const std::filesystem::path& executable, const std::filesystem::path& first, const std::filesystem::path& second) {
    const auto child = fork();
    if (child == 0) {
        execl(executable.c_str(), executable.c_str(), first.c_str(), "-o", second.c_str(), nullptr);
        _exit(127);
    }
    int status = 0;
    return child > 0 && waitpid(child, &status, 0) == child && WIFEXITED(status) ? WEXITSTATUS(status) : 255;
}

int run_output(const std::filesystem::path& executable) {
    const auto child = fork();
    if (child == 0) {
        execl(executable.c_str(), executable.c_str(), nullptr);
        _exit(127);
    }
    int status = 0;
    return child > 0 && waitpid(child, &status, 0) == child && WIFEXITED(status) ? WEXITSTATUS(status) : 255;
}

std::pair<int, std::string> run_text_option(const std::filesystem::path& executable, const std::filesystem::path& input, const char* option) {
    int pipe_fds[2]{};
    if (pipe(pipe_fds) != 0) return {255, {}};
    const auto child = fork();
    if (child == 0) {
        close(pipe_fds[0]);
        dup2(pipe_fds[1], STDOUT_FILENO);
        close(pipe_fds[1]);
        execl(executable.c_str(), executable.c_str(), input.c_str(), option, nullptr);
        _exit(127);
    }
    close(pipe_fds[1]);
    std::string output;
    std::array<char, 128> buffer{};
    for (ssize_t count = 0; (count = read(pipe_fds[0], buffer.data(), buffer.size())) > 0;) output.append(buffer.data(), static_cast<std::size_t>(count));
    close(pipe_fds[0]);
    int status = 0;
    const auto waited = child > 0 ? waitpid(child, &status, 0) : -1;
    return {waited == child && WIFEXITED(status) ? WEXITSTATUS(status) : 255, std::move(output)};
}
}

int main() {
    const auto root = std::filesystem::temp_directory_path() / ("dosrecomp-cli-" + std::to_string(getpid()));
    const auto input = root.string() + ".com";
    const auto output = root.string() + ".elf";
    const auto byte_input = root.string() + "-byte.com";
    const auto byte_output = root.string() + "-byte.elf";
    {
        std::ofstream file(input, std::ios::binary);
        const std::array<unsigned char, 5> program{0xb8, 0x07, 0x4c, 0xcd, 0x21};
        file.write(reinterpret_cast<const char*>(program.data()), static_cast<std::streamsize>(program.size()));
        file.close();
        if (!file) return EXIT_FAILURE;
    }
    {
        std::ofstream file(byte_input, std::ios::binary);
        const std::array<unsigned char, 6> program{0xb4, 0x4c, 0xb0, 0x03, 0xcd, 0x21};
        file.write(reinterpret_cast<const char*>(program.data()), static_cast<std::streamsize>(program.size()));
        file.close();
        if (!file) return EXIT_FAILURE;
    }
    const auto compile_status = run(DOSRECOMP_CLI_PATH, input, output);
    const auto output_status = compile_status == 0 ? run_output(output) : 255;
    const auto byte_compile_status = run(DOSRECOMP_CLI_PATH, byte_input, byte_output);
    const auto byte_output_status = byte_compile_status == 0 ? run_output(byte_output) : 255;
    const auto [dot_status, dot_output] = run_text_option(DOSRECOMP_CLI_PATH, input, "--emit-dot");
    const auto [llvm_status, llvm_output] = run_text_option(DOSRECOMP_CLI_PATH, input, "--emit-llvm");
    std::filesystem::remove(input);
    std::filesystem::remove(output);
    std::filesystem::remove(byte_input);
    std::filesystem::remove(byte_output);
    if (compile_status != 0 || output_status != 7 || byte_compile_status != 0 || byte_output_status != 3 ||
        dot_status != 0 || dot_output.rfind("digraph cfg", 0) != 0 || llvm_status != 0 ||
        llvm_output.find("ret i32 7") == std::string::npos) {
        std::cerr << "CLI recompilation integration test failed\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
