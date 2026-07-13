#include "dosrecomp/plugins/plugin_registry.hpp"

#include <algorithm>

namespace dosrecomp::plugins {
std::expected<void, plugin_error> plugin_registry::register_plugin(plugin_descriptor descriptor) {
    if (descriptor.id.empty() || descriptor.display_name.empty()) return std::unexpected(plugin_error{"plugin ID and display name are required"});
    if (find(descriptor.id)) return std::unexpected(plugin_error{"plugin ID is already registered"});
    plugins_.push_back(std::move(descriptor));
    return {};
}

std::optional<plugin_descriptor> plugin_registry::find(std::string_view id) const {
    const auto found = std::find_if(plugins_.begin(), plugins_.end(), [id](const plugin_descriptor& item) { return item.id == id; });
    return found == plugins_.end() ? std::nullopt : std::optional<plugin_descriptor>{*found};
}

std::vector<plugin_descriptor> plugin_registry::for_extension(extension_point point) const {
    std::vector<plugin_descriptor> result;
    for (const auto& plugin : plugins_) if (plugin.point == point) result.push_back(plugin);
    return result;
}

} // namespace dosrecomp::plugins

