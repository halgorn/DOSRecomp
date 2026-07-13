#include "dosrecomp/runtime/real_mode_memory.hpp"
namespace dosrecomp::runtime {
real_mode_memory::real_mode_memory() : bytes_(size) {}
std::expected<std::uint8_t, memory_error> real_mode_memory::read8(std::uint32_t address) const {
    if (address >= bytes_.size()) return std::unexpected(memory_error{"physical address is outside real-mode memory"});
    return std::to_integer<std::uint8_t>(bytes_[address]);
}
std::expected<void, memory_error> real_mode_memory::write8(std::uint32_t address, std::uint8_t value) {
    if (address >= bytes_.size()) return std::unexpected(memory_error{"physical address is outside real-mode memory"});
    bytes_[address] = static_cast<std::byte>(value); return {};
}
std::expected<std::uint16_t, memory_error> real_mode_memory::read16(std::uint32_t address) const {
    if (address >= bytes_.size() || bytes_.size() - address < 2) return std::unexpected(memory_error{"word crosses real-mode memory boundary"});
    return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes_[address])) |
           (static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes_[address + 1])) << 8U);
}
std::expected<void, memory_error> real_mode_memory::write16(std::uint32_t address, std::uint16_t value) {
    if (address >= bytes_.size() || bytes_.size() - address < 2) return std::unexpected(memory_error{"word crosses real-mode memory boundary"});
    bytes_[address] = static_cast<std::byte>(value); bytes_[address + 1] = static_cast<std::byte>(value >> 8U); return {};
}
std::expected<std::uint8_t, memory_error> real_mode_memory::read8(std::uint16_t segment, std::uint16_t offset) const {
    return read8(physical_address(segment, offset));
}
std::expected<void, memory_error> real_mode_memory::write8(std::uint16_t segment, std::uint16_t offset, std::uint8_t value) {
    return write8(physical_address(segment, offset), value);
}
} // namespace dosrecomp::runtime
