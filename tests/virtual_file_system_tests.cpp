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
    std::filesystem::remove_all(root);
    if (!handle || !data || data->size() != 5 || std::to_integer<unsigned char>((*data)[0]) != 'h' || !closed || traversal) {
        std::cerr << "failed virtual DOS file system\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
