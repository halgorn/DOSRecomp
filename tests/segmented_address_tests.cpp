#include "dosrecomp/runtime/segmented_address.hpp"

#include <cstdlib>

int main() {
    return dosrecomp::runtime::physical_address(0x1234, 0x5678) == 0x179b8 &&
           dosrecomp::runtime::physical_address(0xffff, 0xffff) == 0x10ffef ? EXIT_SUCCESS : EXIT_FAILURE;
}
