#include "dosrecomp/runtime/virtual_file_system.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <unistd.h>

int main() {
    const auto root = std::filesystem::temp_directory_path() / ("dosrecomp-vfs-" + std::to_string(getpid()));
    std::filesystem::create_directory(root);
    {
        std::ofstream file(root / "HELLO.TXT", std::ios::binary);
        file << "hello";
    }
    dosrecomp::runtime::virtual_file_system files{dosrecomp::runtime::virtual_drive(root)};
    const auto handle = files.open_read("C:\\HELLO.TXT");
    const auto data = handle ? files.read(*handle, 8) : std::expected<std::vector<std::byte>, dosrecomp::runtime::file_error>{std::unexpected(dosrecomp::runtime::file_error{"open failed"})};
    const auto closed = handle ? files.close(*handle) : std::expected<void, dosrecomp::runtime::file_error>{std::unexpected(dosrecomp::runtime::file_error{"open failed"})};
    const auto traversal = files.open_read("C:\\..\\secret");
    const auto output = files.open_write("C:\\OUTPUT.BIN");
    const auto written = output ? files.write(*output, {std::byte{0x12}, std::byte{0x34}})
                                : std::expected<std::size_t, dosrecomp::runtime::file_error>{std::unexpected(dosrecomp::runtime::file_error{"open failed"})};
    const auto output_closed = output ? files.close(*output) : std::expected<void, dosrecomp::runtime::file_error>{std::unexpected(dosrecomp::runtime::file_error{"open failed"})};
    std::ifstream output_file(root / "OUTPUT.BIN", std::ios::binary);
    char first = 0;
    output_file.read(&first, 1);
    std::filesystem::remove_all(root);
    if (!handle || !data || data->size() != 5 || std::to_integer<unsigned char>((*data)[0]) != 'h' || !closed || traversal ||
        !output || !written || *written != 2 || !output_closed || first != 0x12) {
        std::cerr << "failed virtual DOS file system\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
