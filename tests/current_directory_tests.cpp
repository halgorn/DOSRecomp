#include "dosrecomp/runtime/current_directory.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <unistd.h>

int main() {
    const auto root = std::filesystem::temp_directory_path() / ("dosrecomp-cwd-" + std::to_string(getpid()));
    std::filesystem::create_directories(root / "GAME");
    dosrecomp::runtime::current_directory directory{dosrecomp::runtime::virtual_drive(root)};
    const auto changed = directory.change_to("C:\\GAME");
    const auto missing = directory.change_to("C:\\MISSING");
    const auto traversal = directory.change_to("C:\\..\\outside");
    std::filesystem::remove_all(root);
    if (!changed || directory.path() != "C:\\GAME" || missing || traversal) {
        std::cerr << "failed DOS current directory state\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

