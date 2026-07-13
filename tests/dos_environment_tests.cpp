#include "dosrecomp/runtime/dos_environment.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    dosrecomp::runtime::dos_environment environment;
    const auto set = environment.set("path", "C:\\GAME");
    const auto value = environment.get("PATH");
    const auto erased = environment.erase("PaTh");
    const auto invalid = environment.set("BAD=KEY", "value");
    if (!set || !value || *value != "C:\\GAME" || !erased || environment.get("path") || invalid) {
        std::cerr << "failed DOS environment store\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

