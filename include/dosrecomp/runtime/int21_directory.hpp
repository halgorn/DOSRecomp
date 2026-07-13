/** @file int21_directory.hpp @brief Current-directory subset of DOS INT 21h. */
#pragma once

#include "dosrecomp/runtime/current_directory.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace dosrecomp::runtime {

struct int21_directory_request { std::uint8_t ah{}; std::string_view path{}; };
struct int21_directory_result { std::string path; };
struct int21_directory_error { std::string message; };

/** Implements DOS change-directory (`3Bh`) and get-current-directory (`47h`). */
class int21_directory_dispatcher final {
public:
    [[nodiscard]] static std::expected<int21_directory_result, int21_directory_error>
    dispatch(const int21_directory_request& request, current_directory& directory);
};

} // namespace dosrecomp::runtime

