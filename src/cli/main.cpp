#include "dosrecomp/loader/binary_loader.hpp"

#include <iostream>
#include <string_view>

namespace {
void print_usage() {
    std::cerr << "Usage: dosrecomp <input.com|input.exe> [--verbose]\n";
}
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        print_usage();
        return 2;
    }
    const bool verbose = argc == 3 && std::string_view(argv[2]) == "--verbose";
    if (argc == 3 && !verbose) {
        print_usage();
        return 2;
    }
    const auto result = dosrecomp::loader::binary_loader::load_file(argv[1]);
    if (!result) {
        std::cerr << "dosrecomp: " << result.error().message << '\n';
        return 1;
    }
    if (verbose) {
        const auto& image = *result;
        std::cout << "format: " << (image.format == dosrecomp::loader::executable_format::com ? "COM" : "MZ")
                  << "\nload module bytes: " << image.bytes.size()
                  << "\nrelocations: " << image.relocations.size() << '\n';
    }
    std::cout << "Loaded successfully. Recompilation pipeline is not implemented yet.\n";
}

