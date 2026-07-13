/** @file plugin_registry.hpp @brief Typed registry for optional DOSRecomp extension points. */
#pragma once

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dosrecomp::plugins {

enum class extension_point { decoder, optimizer, api_mapping, loader, output_generator };
struct plugin_descriptor { std::string id; std::string display_name; extension_point point{}; };
struct plugin_error { std::string message; };

/** Owns plugin metadata and rejects duplicate identifiers across extension points. */
class plugin_registry final {
public:
    [[nodiscard]] std::expected<void, plugin_error> register_plugin(plugin_descriptor descriptor);
    [[nodiscard]] std::optional<plugin_descriptor> find(std::string_view id) const;
    [[nodiscard]] std::vector<plugin_descriptor> for_extension(extension_point point) const;

private:
    std::vector<plugin_descriptor> plugins_;
};

} // namespace dosrecomp::plugins

