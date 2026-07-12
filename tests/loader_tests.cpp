#include "dosrecomp/loader/binary_loader.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

using dosrecomp::loader::binary_loader;

namespace {
std::byte b(unsigned value) { return static_cast<std::byte>(value); }

bool expect(bool condition, std::string_view name) {
    if (!condition) {
        std::cerr << "FAILED: " << name << '\n';
    }
    return condition;
}

bool test_com() {
    const auto result = binary_loader::load_bytes({b(0xb8), b(0x00), b(0x4c), b(0xcd), b(0x21)});
    return expect(result.has_value(), "load valid COM") &&
           expect(result->bytes.size() == 5, "preserve COM bytes") &&
           expect(result->entry_point.offset == 0x100, "assign COM origin") &&
           expect(result->entry_offset() == 0, "map COM origin to image offset");
}

bool test_mz_with_relocation() {
    std::vector<std::byte> file(48, b(0));
    file[0] = b('M'); file[1] = b('Z'); file[2] = b(48); file[4] = b(1);
    file[6] = b(1); file[8] = b(2); file[20] = b(0); file[22] = b(0); file[24] = b(28);
    file[28] = b(0); file[29] = b(0); file[30] = b(0); file[31] = b(0);
    const auto result = binary_loader::load_bytes(file);
    return expect(result.has_value(), "load valid MZ") &&
           expect(result->bytes.size() == 16, "strip MZ header") &&
           expect(result->relocations.size() == 1, "parse relocation") &&
           expect(result->format == dosrecomp::loader::executable_format::mz, "identify MZ") &&
           expect(result->entry_offset() == 0, "map MZ entry to image offset");
}

bool test_reject_invalid_inputs() {
    std::vector<std::byte> truncated{b('M'), b('Z')};
    std::vector<std::byte> oversized(65536, b(0));
    std::vector<std::byte> bad_relocation(32, b(0));
    bad_relocation[0] = b('M'); bad_relocation[1] = b('Z'); bad_relocation[2] = b(32);
    bad_relocation[4] = b(1); bad_relocation[6] = b(1); bad_relocation[8] = b(2); bad_relocation[24] = b(31);
    return expect(!binary_loader::load_bytes(truncated), "reject truncated MZ") &&
           expect(!binary_loader::load_bytes(oversized), "reject oversized COM") &&
           expect(!binary_loader::load_bytes(bad_relocation), "reject relocation outside header");
}
}

int main() {
    const bool passed = test_com() && test_mz_with_relocation() && test_reject_invalid_inputs();
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
