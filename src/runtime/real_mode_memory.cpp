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
} // namespace dosrecomp::runtime
