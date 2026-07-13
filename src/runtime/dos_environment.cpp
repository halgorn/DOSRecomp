#include "dosrecomp/runtime/dos_environment.hpp"

#include <cctype>

namespace dosrecomp::runtime {
std::expected<std::string, environment_error> dos_environment::normalize_key(std::string_view key) {
    if (key.empty()) return std::unexpected(environment_error{"DOS environment key is empty"});
    std::string result;
    result.reserve(key.size());
    for (const auto character : key) {
        if (character == '=' || character == '\0') return std::unexpected(environment_error{"DOS environment key contains an invalid character"});
        result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(character))));
    }
    return result;
}

std::expected<void, environment_error> dos_environment::set(std::string_view key, std::string_view value) {
    const auto normalized = normalize_key(key);
    if (!normalized) return std::unexpected(normalized.error());
    if (value.find('\0') != std::string_view::npos) return std::unexpected(environment_error{"DOS environment value contains an invalid character"});
    variables_[*normalized] = value;
    return {};
}

std::optional<std::string> dos_environment::get(std::string_view key) const {
    const auto normalized = normalize_key(key);
    if (!normalized) return std::nullopt;
    const auto found = variables_.find(*normalized);
    return found == variables_.end() ? std::nullopt : std::optional<std::string>{found->second};
}

std::expected<void, environment_error> dos_environment::erase(std::string_view key) {
    const auto normalized = normalize_key(key);
    if (!normalized) return std::unexpected(normalized.error());
    variables_.erase(*normalized);
    return {};
}

} // namespace dosrecomp::runtime

