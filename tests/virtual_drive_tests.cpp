#include "dosrecomp/runtime/virtual_drive.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    const dosrecomp::runtime::virtual_drive drive("/sandbox/game");
    const auto valid = drive.resolve("c:\\DATA/LEVEL1.DAT");
    const auto traversal = drive.resolve("C:\\DATA\\..\\secret");
    const auto wrong_drive = drive.resolve("D:\\DATA");
    if (!valid || *valid != std::filesystem::path("/sandbox/game/DATA/LEVEL1.DAT") || traversal || wrong_drive) {
        std::cerr << "failed virtual drive path resolution\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

