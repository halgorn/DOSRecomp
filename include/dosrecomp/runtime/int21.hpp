/** @file int21.hpp @brief Safe host-side implementations of the initial DOS INT 21h services. */
#pragma once

#include "dosrecomp/runtime/conventional_memory.hpp"

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
    std::uint16_t bx{};
    std::uint16_t es{};
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

/** Result registers of the DOS conventional-memory services. */
struct int21_memory_result { std::optional<std::uint16_t> allocated_segment; };

/** Implements INT 21h allocation (`48h`) and release (`49h`) through conventional memory. */
class int21_memory_dispatcher final {
public:
    [[nodiscard]] static std::expected<int21_memory_result, dos_error>
    dispatch(const int21_request& request, conventional_memory& memory);
};

/** DOS-compatible clock values supplied by the host integration layer. */
struct dos_clock { std::uint16_t year{}; std::uint8_t month{}; std::uint8_t day{}; std::uint8_t weekday{}; std::uint8_t hour{}; std::uint8_t minute{}; std::uint8_t second{}; std::uint8_t hundredths{}; };
struct int21_clock_result { std::optional<dos_clock> value; };

/** Implements INT 21h date (`2Ah`) and time (`2Ch`) queries from an injected clock. */
class int21_clock_dispatcher final {
public:
    [[nodiscard]] static std::expected<int21_clock_result, dos_error>
    dispatch(const int21_request& request, const dos_clock& clock);
};

} // namespace dosrecomp::runtime
