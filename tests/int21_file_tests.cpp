#include "dosrecomp/runtime/int21_file.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <unistd.h>

int main() {
    const auto root = std::filesystem::temp_directory_path() / ("dosrecomp-int21-file-" + std::to_string(getpid()));
    std::filesystem::create_directory(root);
    { std::ofstream(root / "DATA.BIN", std::ios::binary) << "abc"; }
    dosrecomp::runtime::virtual_file_system files{dosrecomp::runtime::virtual_drive(root)};
    const auto opened = dosrecomp::runtime::int21_file_dispatcher::dispatch({.ah = 0x3d, .path = "C:\\DATA.BIN"}, files);
    const auto read = opened && opened->handle
        ? dosrecomp::runtime::int21_file_dispatcher::dispatch({.ah = 0x3f, .handle = *opened->handle, .count = 8}, files)
        : std::expected<dosrecomp::runtime::int21_file_result, dosrecomp::runtime::int21_file_error>{std::unexpected(dosrecomp::runtime::int21_file_error{"open failed"})};
    const auto close = opened && opened->handle
        ? dosrecomp::runtime::int21_file_dispatcher::dispatch({.ah = 0x3e, .handle = *opened->handle}, files)
        : std::expected<dosrecomp::runtime::int21_file_result, dosrecomp::runtime::int21_file_error>{std::unexpected(dosrecomp::runtime::int21_file_error{"open failed"})};
    std::filesystem::remove_all(root);
    if (!opened || !opened->handle || !read || read->data.size() != 3 || std::to_integer<unsigned char>(read->data[0]) != 'a' || !close) {
        std::cerr << "failed INT 21h file dispatch\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

