/** @file dos_environment.hpp @brief Isolated DOS environment-variable store. */
#pragma once

#include <expected>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace dosrecomp::runtime {

struct environment_error { std::string message; };

/** Stores guest environment values without inheriting host process variables. */
class dos_environment final {
public:
    [[nodiscard]] std::expected<void, environment_error> set(std::string_view key, std::string_view value);
    [[nodiscard]] std::optional<std::string> get(std::string_view key) const;
    [[nodiscard]] std::expected<void, environment_error> erase(std::string_view key);

private:
    [[nodiscard]] static std::expected<std::string, environment_error> normalize_key(std::string_view key);
    std::map<std::string, std::string> variables_;
};

} // namespace dosrecomp::runtime

