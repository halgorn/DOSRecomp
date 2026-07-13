/** @file current_directory.hpp @brief Sandboxed current-directory state for DOS programs. */
#pragma once

#include "dosrecomp/runtime/virtual_drive.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace dosrecomp::runtime {

struct directory_error { std::string message; };

/** Tracks a guest-visible current DOS directory after validating it against a virtual drive. */
class current_directory final {
public:
    explicit current_directory(virtual_drive drive);

    [[nodiscard]] std::expected<void, directory_error> change_to(std::string_view dos_path);
    [[nodiscard]] const std::string& path() const noexcept;

private:
    virtual_drive drive_;
    std::string path_{"C:\\"};
};

} // namespace dosrecomp::runtime

