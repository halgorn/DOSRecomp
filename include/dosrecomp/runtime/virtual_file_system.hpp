/** @file virtual_file_system.hpp @brief Read-only DOS file handles over a sandboxed virtual drive. */
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

/** Manages DOS-style read handles, beginning at handle 5, within one virtual drive. */
class virtual_file_system final {
public:
    explicit virtual_file_system(virtual_drive drive);

    [[nodiscard]] std::expected<std::uint16_t, file_error> open_read(std::string_view dos_path);
    [[nodiscard]] std::expected<std::vector<std::byte>, file_error> read(std::uint16_t handle, std::size_t count);
    [[nodiscard]] std::expected<void, file_error> close(std::uint16_t handle);

private:
    virtual_drive drive_;
    std::map<std::uint16_t, std::unique_ptr<std::ifstream>> files_;
};

} // namespace dosrecomp::runtime

