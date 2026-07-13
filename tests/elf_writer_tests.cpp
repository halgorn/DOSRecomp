#include "dosrecomp/backend/elf_writer.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    const auto image = dosrecomp::backend::elf_writer::emit_exit_executable(7);
    const auto byte = [&](std::size_t index) { return std::to_integer<unsigned char>(image[index]); };
    if (image.size() != 132 || byte(0) != 0x7f || byte(1) != 'E' || byte(4) != 2 || byte(18) != 62 ||
        byte(120) != 0xb8 || byte(125) != 0xbf || byte(126) != 7 || byte(130) != 0x0f || byte(131) != 0x05) {
        std::cerr << "failed ELF64 image emission\n";
        return EXIT_FAILURE;
    }
    const auto path = std::filesystem::temp_directory_path() / ("dosrecomp-elf-" + std::to_string(getpid()));
    {
        std::ofstream output(path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(image.data()), static_cast<std::streamsize>(image.size()));
        if (!output) return EXIT_FAILURE;
    }
    if (chmod(path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0) return EXIT_FAILURE;
    const auto child = fork();
    if (child == 0) {
        execl(path.c_str(), path.c_str(), nullptr);
        _exit(127);
    }
    int status = 0;
    const auto waited = child > 0 ? waitpid(child, &status, 0) : -1;
    std::filesystem::remove(path);
    if (waited != child || !WIFEXITED(status) || WEXITSTATUS(status) != 7) {
        std::cerr << "generated ELF did not execute with expected status\n";
        return EXIT_FAILURE;
    }
    const std::vector<std::byte> hello{std::byte{'h'}, std::byte{'i'}, std::byte{'\n'}};
    const std::vector<std::byte> bye{std::byte{'b'}, std::byte{'y'}, std::byte{'e'}};
    const dosrecomp::backend::write_call calls[2]{{std::span<const std::byte>{hello}, 1}, {std::span<const std::byte>{bye}, 2}};
    const auto multi = dosrecomp::backend::elf_writer::emit_multi_write_exit_executable(std::span<const dosrecomp::backend::write_call>{calls, 2}, 0);
    const auto multi_path = std::filesystem::temp_directory_path() / ("dosrecomp-elf-multi-" + std::to_string(getpid()));
    {
        std::ofstream output(multi_path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(multi.data()), static_cast<std::streamsize>(multi.size()));
    }
    if (chmod(multi_path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0) return EXIT_FAILURE;
    const auto out_path = multi_path.string() + ".out";
    const auto err_path = multi_path.string() + ".err";
    const auto cmd = multi_path.string() + " > " + out_path + " 2> " + err_path;
    const auto multi_status = std::system(cmd.c_str());
    std::ifstream stdout_file(out_path, std::ios::binary);
    std::ifstream stderr_file(err_path, std::ios::binary);
    std::string stdout_text{std::istreambuf_iterator<char>{stdout_file}, {}};
    std::string stderr_text{std::istreambuf_iterator<char>{stderr_file}, {}};
    std::filesystem::remove(multi_path);
    std::filesystem::remove(out_path);
    std::filesystem::remove(err_path);
    if (multi_status == -1 || !WIFEXITED(multi_status) || WEXITSTATUS(multi_status) != 0 ||
        stdout_text != "hi\n" || stderr_text != "bye") {
        std::cerr << "multi-write ELF did not produce expected streams\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
