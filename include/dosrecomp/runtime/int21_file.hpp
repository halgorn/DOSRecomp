/** @file int21_file.hpp @brief File-I/O subset of DOS INT 21h over virtual handles. */
#pragma once

#include "dosrecomp/runtime/virtual_file_system.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dosrecomp::runtime {

struct int21_file_request { std::uint8_t ah{}; std::string_view path{}; std::uint16_t handle{}; std::size_t count{}; };
struct int21_file_result { std::optional<std::uint16_t> handle; std::vector<std::byte> data; };
struct int21_file_error { std::string message; };

/** Implements DOS open (`3Dh`), read (`3Fh`), and close (`3Eh`) functions. */
class int21_file_dispatcher final {
public:
    [[nodiscard]] static std::expected<int21_file_result, int21_file_error>
    dispatch(const int21_file_request& request, virtual_file_system& files);
};

} // namespace dosrecomp::runtime

