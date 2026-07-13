#include "dosrecomp/runtime/int13.hpp"

#include <cstdlib>
#include <iostream>

namespace { std::byte b(unsigned value) { return static_cast<std::byte>(value); } }

int main() {
    dosrecomp::runtime::virtual_disk disk{{.cylinders = 1, .heads = 1, .sectors_per_track = 2}, std::vector<std::byte>(1024, b(0))};
    disk.bytes[512] = b(0x5a);
    const auto read = dosrecomp::runtime::int13_dispatcher::dispatch({.ah = 0x02, .al = 1, .cl = 2, .dl = 0x80}, disk);
    const auto invalid = dosrecomp::runtime::int13_dispatcher::dispatch({.ah = 0x02, .al = 1, .cl = 3, .dl = 0x80}, disk);
    if (!read || read->sectors_transferred != 1 || std::to_integer<unsigned char>(read->data[0]) != 0x5a || invalid) {
        std::cerr << "failed INT 13h virtual disk dispatch\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

