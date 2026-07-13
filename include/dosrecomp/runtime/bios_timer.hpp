/** @file bios_timer.hpp @brief Deterministic BIOS tick timer and INT 1Ah time service. */
#pragma once

#include <cstdint>
#include <expected>
#include <string>

namespace dosrecomp::runtime {

/** Caller-owned state for the BIOS 18.2 Hz tick counter. */
class bios_timer final {
public:
    static constexpr std::uint32_t ticks_per_day = 1'573'040;

    explicit bios_timer(std::uint32_t ticks = 0) noexcept;

    /** Advances the timer, retaining the count modulo one BIOS day. */
    void advance(std::uint32_t ticks) noexcept;
    [[nodiscard]] std::uint32_t ticks() const noexcept;
    /** Returns and clears the number of midnight rollovers, capped at 255. */
    [[nodiscard]] std::uint8_t take_midnight_rollovers() noexcept;

private:
    std::uint32_t ticks_{};
    std::uint8_t midnight_rollovers_{};
};

struct int1a_request { std::uint8_t ah{}; };
struct int1a_result { std::uint32_t ticks{}; std::uint8_t midnight_rollovers{}; };
struct timer_error { std::string message; };

/** Implements BIOS INT 1Ah get-system-time (`AH=00h`). */
class int1a_dispatcher final {
public:
    [[nodiscard]] static std::expected<int1a_result, timer_error>
    dispatch(const int1a_request& request, bios_timer& timer);
};

} // namespace dosrecomp::runtime
