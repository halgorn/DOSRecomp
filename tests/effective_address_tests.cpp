#include "dosrecomp/decoder/effective_address.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    using namespace dosrecomp::decoder;
    operand indexed{.kind = operand_kind::memory, .address_registers = {register_name::bx, register_name::si}, .address_register_count = 2, .displacement = -2};
    register_values registers{};
    registers[static_cast<std::size_t>(register_name::bx)] = 3;
    registers[static_cast<std::size_t>(register_name::si)] = 4;
    operand direct{.kind = operand_kind::memory, .displacement = -1, .direct_address = true};
    const auto resolved = effective_address_resolver::resolve(indexed, registers);
    const auto absolute = effective_address_resolver::resolve(direct, registers);
    if (!resolved || *resolved != 5 || !absolute || *absolute != 0xffff) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
