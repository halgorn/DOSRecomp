#include "dosrecomp/runtime/virtual_drive.hpp"

#include <cctype>
#include <string>

namespace dosrecomp::runtime {
namespace {
[[nodiscard]] char upper(char character) { return static_cast<char>(std::toupper(static_cast<unsigned char>(character))); }
}

virtual_drive::virtual_drive(std::filesystem::path root, char drive) : root_(std::move(root)), drive_(upper(drive)) {}

std::expected<std::filesystem::path, path_error> virtual_drive::resolve(std::string_view dos_path) const {
    if (dos_path.size() < 3 || dos_path[1] != ':' || upper(dos_path[0]) != drive_ ||
        (dos_path[2] != '\\' && dos_path[2] != '/')) {
        return std::unexpected(path_error{"path must be absolute on the configured DOS drive"});
    }
    auto result = root_;
    std::string component;
    for (std::size_t index = 3; index <= dos_path.size(); ++index) {
        const bool boundary = index == dos_path.size() || dos_path[index] == '\\' || dos_path[index] == '/';
        if (!boundary) {
            component.push_back(dos_path[index]);
            continue;
        }
        if (!component.empty()) {
            if (component == "." || component == "..") return std::unexpected(path_error{"DOS path traversal is not allowed"});
            result /= component;
            component.clear();
        }
    }
    std::error_code error;
    const auto canonical_root = std::filesystem::weakly_canonical(root_, error);
    if (error) return std::unexpected(path_error{"configured DOS drive root cannot be resolved"});
    const auto canonical_result = std::filesystem::weakly_canonical(result, error);
    if (error) return std::unexpected(path_error{"DOS path cannot be resolved safely"});
    const auto relative = canonical_result.lexically_relative(canonical_root);
    if (relative.empty() && canonical_result != canonical_root) {
        return std::unexpected(path_error{"DOS path escapes the virtual drive"});
    }
    for (const auto& component_path : relative) {
        if (component_path == "..") return std::unexpected(path_error{"DOS path escapes the virtual drive"});
    }
    return canonical_result;
}

} // namespace dosrecomp::runtime
