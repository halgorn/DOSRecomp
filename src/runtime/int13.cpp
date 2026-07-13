#include "dosrecomp/runtime/int13.hpp"

namespace dosrecomp::runtime {
std::expected<int13_result, disk_error>
int13_dispatcher::dispatch(const int13_request& request, const virtual_disk& disk) {
    constexpr std::size_t sector_size = 512;
    if (request.ah != 0x02) return std::unexpected(disk_error{"unsupported INT 13h function"});
    if (request.dl != 0x80) return std::unexpected(disk_error{"virtual disk is mapped only as drive 80h"});
    const auto sector = static_cast<std::uint8_t>(request.cl & 0x3fU);
    const auto cylinder = static_cast<std::uint16_t>(request.ch) | (static_cast<std::uint16_t>(request.cl & 0xc0U) << 2U);
    if (request.al == 0 || sector == 0 || cylinder >= disk.geometry.cylinders || request.dh >= disk.geometry.heads ||
        sector > disk.geometry.sectors_per_track) return std::unexpected(disk_error{"INT 13h CHS request is outside geometry"});
    const auto lba = (static_cast<std::size_t>(cylinder) * disk.geometry.heads + request.dh) * disk.geometry.sectors_per_track + (sector - 1U);
    const auto offset = lba * sector_size;
    const auto length = static_cast<std::size_t>(request.al) * sector_size;
    if (offset > disk.bytes.size() || length > disk.bytes.size() - offset) return std::unexpected(disk_error{"INT 13h request exceeds disk image"});
    return int13_result{{disk.bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                         disk.bytes.begin() + static_cast<std::ptrdiff_t>(offset + length)}, request.al};
}

} // namespace dosrecomp::runtime

