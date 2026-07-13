#include "dosrecomp/plugins/plugin_registry.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    dosrecomp::plugins::plugin_registry registry;
    const auto decoder = registry.register_plugin({.id = "x86.extra", .display_name = "Extra x86", .point = dosrecomp::plugins::extension_point::decoder});
    const auto output = registry.register_plugin({.id = "emit.c", .display_name = "C output", .point = dosrecomp::plugins::extension_point::output_generator});
    const auto duplicate = registry.register_plugin({.id = "x86.extra", .display_name = "Duplicate", .point = dosrecomp::plugins::extension_point::decoder});
    if (!decoder || !output || duplicate || !registry.find("emit.c") || registry.for_extension(dosrecomp::plugins::extension_point::decoder).size() != 1) {
        std::cerr << "failed plugin registry\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

