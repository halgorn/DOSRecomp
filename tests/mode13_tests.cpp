#include "dosrecomp/runtime/mode13.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    dosrecomp::runtime::mode13_framebuffer framebuffer;
    const auto set = framebuffer.set_pixel(319, 199, 42);
    const auto pixel = framebuffer.pixel(319, 199);
    const auto invalid = framebuffer.set_pixel(320, 0, 1);
    framebuffer.set_palette(42, {.red = 1, .green = 2, .blue = 3});
    const auto color = framebuffer.palette(42);
    if (!set || !pixel || *pixel != 42 || invalid || color.red != 1 || color.green != 2 || color.blue != 3) {
        std::cerr << "failed Mode 13h framebuffer\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

