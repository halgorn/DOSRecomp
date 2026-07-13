#include "dosrecomp/runtime/virtual_drive.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <unistd.h>

int main() {
    const auto root = std::filesystem::temp_directory_path() / ("dosrecomp-drive-" + std::to_string(getpid()));
    const auto outside = std::filesystem::temp_directory_path() / ("dosrecomp-outside-" + std::to_string(getpid()));
    std::filesystem::create_directories(root / "DATA");
    std::filesystem::create_directories(outside);
    std::filesystem::create_directory_symlink(outside, root / "ESCAPE");
    const dosrecomp::runtime::virtual_drive drive(root);
    const auto valid = drive.resolve("c:\\DATA/LEVEL1.DAT");
    const auto traversal = drive.resolve("C:\\DATA\\..\\secret");
    const auto wrong_drive = drive.resolve("D:\\DATA");
    const auto escape = drive.resolve("C:\\ESCAPE\\secret");
    std::filesystem::remove_all(root);
    std::filesystem::remove_all(outside);
    if (!valid || *valid != std::filesystem::weakly_canonical(root / "DATA" / "LEVEL1.DAT") || traversal || wrong_drive || escape) {
        std::cerr << "failed virtual drive path resolution\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
