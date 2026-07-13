#include "dosrecomp/decoder/effective_address.hpp"

namespace dosrecomp::decoder {
std::expected<std::uint16_t, effective_address_error>
effective_address_resolver::resolve(const operand& memory, const register_values& registers) {
    if (memory.kind != operand_kind::memory) return std::unexpected(effective_address_error{"operand is not memory"});
    if (memory.direct_address) return static_cast<std::uint16_t>(memory.displacement);
    std::uint16_t offset = static_cast<std::uint16_t>(memory.displacement);
    for (std::uint8_t index = 0; index < memory.address_register_count; ++index) {
        offset = static_cast<std::uint16_t>(offset + registers[static_cast<std::size_t>(memory.address_registers[index])]);
    }
    return offset;
}
} // namespace dosrecomp::decoder
