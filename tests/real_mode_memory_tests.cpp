#include "dosrecomp/runtime/real_mode_memory.hpp"
#include <cstdlib>
int main() { dosrecomp::runtime::real_mode_memory memory; const auto write = memory.write8(0x10ffef, 0xaa); const auto read = memory.read8(0x10ffef); const auto word = memory.write16(2, 0x1234); const auto word_read = memory.read16(2); return write && read && *read == 0xaa && word && word_read && *word_read == 0x1234 && !memory.read8(0x110000) && !memory.read16(0x10ffff) ? EXIT_SUCCESS : EXIT_FAILURE; }
