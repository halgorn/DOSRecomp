#include "dosrecomp/runtime/real_mode_memory.hpp"
#include <cstdlib>
int main() { dosrecomp::runtime::real_mode_memory memory; const auto write = memory.write8(0x10ffef, 0xaa); const auto read = memory.read8(0x10ffef); return write && read && *read == 0xaa && !memory.read8(0x110000) ? EXIT_SUCCESS : EXIT_FAILURE; }
