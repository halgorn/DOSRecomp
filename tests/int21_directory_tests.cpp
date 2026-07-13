#include "dosrecomp/runtime/int21_directory.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <unistd.h>

int main() {
    const auto root = std::filesystem::temp_directory_path() / ("dosrecomp-int21-dir-" + std::to_string(getpid()));
    std::filesystem::create_directories(root / "GAME");
    dosrecomp::runtime::current_directory directory{dosrecomp::runtime::virtual_drive(root)};
    const auto changed = dosrecomp::runtime::int21_directory_dispatcher::dispatch({.ah = 0x3b, .path = "C:\\GAME"}, directory);
    const auto current = dosrecomp::runtime::int21_directory_dispatcher::dispatch({.ah = 0x47}, directory);
    std::filesystem::remove_all(root);
    if (!changed || changed->path != "C:\\GAME" || !current || current->path != "C:\\GAME") {
        std::cerr << "failed INT 21h directory dispatch\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

