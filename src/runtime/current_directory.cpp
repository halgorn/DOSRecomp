#include "dosrecomp/runtime/current_directory.hpp"

#include <filesystem>

namespace dosrecomp::runtime {
current_directory::current_directory(virtual_drive drive) : drive_(std::move(drive)) {}

std::expected<void, directory_error> current_directory::change_to(std::string_view dos_path) {
    const auto resolved = drive_.resolve(dos_path);
    if (!resolved) return std::unexpected(directory_error{resolved.error().message});
    std::error_code error;
    if (!std::filesystem::is_directory(*resolved, error) || error) {
        return std::unexpected(directory_error{"DOS current directory does not exist"});
    }
    path_ = dos_path;
    return {};
}

const std::string& current_directory::path() const noexcept { return path_; }

} // namespace dosrecomp::runtime

