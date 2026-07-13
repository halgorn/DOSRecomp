/** @file virtual_file_system.hpp @brief Sandboxed DOS file handles over a virtual drive. */
#pragma once

#include "dosrecomp/runtime/virtual_drive.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace dosrecomp::runtime {

struct file_error { std::string message; };

/** Manages DOS-style read/write handles, beginning at handle 5, within one virtual drive. */
class virtual_file_system final {
public:
    explicit virtual_file_system(virtual_drive drive);

    [[nodiscard]] std::expected<std::uint16_t, file_error> open_read(std::string_view dos_path);
    [[nodiscard]] std::expected<std::uint16_t, file_error> open_write(std::string_view dos_path);
    [[nodiscard]] std::expected<std::vector<std::byte>, file_error> read(std::uint16_t handle, std::size_t count);
    [[nodiscard]] std::expected<std::size_t, file_error> write(std::uint16_t handle, const std::vector<std::byte>& data);
    [[nodiscard]] std::expected<void, file_error> close(std::uint16_t handle);

private:
    virtual_drive drive_;
    struct file_handle { std::unique_ptr<std::fstream> stream; bool readable{}; bool writable{}; };
    std::map<std::uint16_t, file_handle> files_;
};

} // namespace dosrecomp::runtime
