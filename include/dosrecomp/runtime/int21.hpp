/** @file int21.hpp @brief Safe host-side implementations of the initial DOS INT 21h services. */
#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace dosrecomp::runtime {

/** Registers consumed by the currently implemented INT 21h functions. */
struct int21_request {
    std::uint8_t ah{};
    std::uint8_t al{};
    std::uint8_t dl{};
    std::size_t string_offset{};
};

/** Observable state of translated console and termination services. */
struct dos_process_state {
    std::string console_output;
    std::optional<std::uint8_t> exit_code;
};

/** A service function that cannot safely be translated. */
struct dos_error { std::string message; };

/** Dispatches supported INT 21h services without accessing host global state. */
class int21_dispatcher final {
public:
    [[nodiscard]] static std::expected<void, dos_error>
    dispatch(const int21_request& request, const std::vector<std::byte>& memory, dos_process_state& state);
};

} // namespace dosrecomp::runtime

