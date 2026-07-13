#include "dosrecomp/runtime/virtual_file_system.hpp"

#include <limits>

namespace dosrecomp::runtime {
virtual_file_system::virtual_file_system(virtual_drive drive) : drive_(std::move(drive)) {}

std::expected<std::uint16_t, file_error> virtual_file_system::open_read(std::string_view dos_path) {
    const auto path = drive_.resolve(dos_path);
    if (!path) return std::unexpected(file_error{path.error().message});
    auto file = std::make_unique<std::fstream>(*path, std::ios::binary | std::ios::in);
    if (!*file) return std::unexpected(file_error{"cannot open virtual DOS file"});
    std::uint16_t handle = 5;
    while (files_.contains(handle)) {
        if (handle == std::numeric_limits<std::uint16_t>::max()) return std::unexpected(file_error{"DOS handle table is exhausted"});
        ++handle;
    }
    files_.emplace(handle, file_handle{.stream = std::move(file), .readable = true, .writable = false});
    return handle;
}

std::expected<std::uint16_t, file_error> virtual_file_system::open_write(std::string_view dos_path) {
    const auto path = drive_.resolve(dos_path);
    if (!path) return std::unexpected(file_error{path.error().message});
    auto file = std::make_unique<std::fstream>(*path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!*file) return std::unexpected(file_error{"cannot create virtual DOS file"});
    std::uint16_t handle = 5;
    while (files_.contains(handle)) {
        if (handle == std::numeric_limits<std::uint16_t>::max()) return std::unexpected(file_error{"DOS handle table is exhausted"});
        ++handle;
    }
    files_.emplace(handle, file_handle{.stream = std::move(file), .readable = false, .writable = true});
    return handle;
}

std::expected<std::vector<std::byte>, file_error> virtual_file_system::read(std::uint16_t handle, std::size_t count) {
    const auto found = files_.find(handle);
    if (found == files_.end()) return std::unexpected(file_error{"DOS file handle is invalid"});
    if (!found->second.readable) return std::unexpected(file_error{"DOS file handle is not open for reading"});
    std::vector<std::byte> data(count);
    found->second.stream->read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(count));
    const auto read_count = found->second.stream->gcount();
    if (found->second.stream->bad()) return std::unexpected(file_error{"cannot read virtual DOS file"});
    data.resize(static_cast<std::size_t>(read_count));
    return data;
}

std::expected<std::size_t, file_error> virtual_file_system::write(std::uint16_t handle, const std::vector<std::byte>& data) {
    const auto found = files_.find(handle);
    if (found == files_.end()) return std::unexpected(file_error{"DOS file handle is invalid"});
    if (!found->second.writable) return std::unexpected(file_error{"DOS file handle is not open for writing"});
    found->second.stream->write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!*found->second.stream) return std::unexpected(file_error{"cannot write virtual DOS file"});
    return data.size();
}

std::expected<void, file_error> virtual_file_system::close(std::uint16_t handle) {
    const auto removed = files_.erase(handle);
    if (removed == 0) return std::unexpected(file_error{"DOS file handle is invalid"});
    return {};
}

} // namespace dosrecomp::runtime
