/** @file virtual_drive.hpp @brief Sandboxed conversion of absolute DOS paths to host paths. */
#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

namespace dosrecomp::runtime {

/** A validation error while resolving a guest DOS path. */
struct path_error { std::string message; };

/**
 * Maps one DOS drive letter to a host root without allowing guest traversal.
 *
 * This class performs lexical mapping only; callers decide whether to create,
 * read, or write the resulting path.
 */
class virtual_drive final {
public:
    explicit virtual_drive(std::filesystem::path root, char drive = 'C');

    [[nodiscard]] std::expected<std::filesystem::path, path_error>
    resolve(std::string_view dos_path) const;

private:
    std::filesystem::path root_;
    char drive_;
};

} // namespace dosrecomp::runtime
