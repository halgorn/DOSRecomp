#include "dosrecomp/runtime/bios_timer.hpp"

#include <algorithm>
#include <limits>

namespace dosrecomp::runtime {
bios_timer::bios_timer(std::uint32_t ticks) noexcept : ticks_(ticks % ticks_per_day) {}

void bios_timer::advance(std::uint32_t ticks) noexcept {
    const auto total = static_cast<std::uint64_t>(ticks_) + ticks;
    const auto days = total / ticks_per_day;
    ticks_ = static_cast<std::uint32_t>(total % ticks_per_day);
    const auto available = static_cast<std::uint16_t>(std::numeric_limits<std::uint8_t>::max() - midnight_rollovers_);
    midnight_rollovers_ = static_cast<std::uint8_t>(midnight_rollovers_ + std::min<std::uint64_t>(days, available));
}

std::uint32_t bios_timer::ticks() const noexcept { return ticks_; }

std::uint8_t bios_timer::take_midnight_rollovers() noexcept {
    const auto rollovers = midnight_rollovers_;
    midnight_rollovers_ = 0;
    return rollovers;
}

std::expected<int1a_result, timer_error>
int1a_dispatcher::dispatch(const int1a_request& request, bios_timer& timer) {
    if (request.ah != 0x00) return std::unexpected(timer_error{"unsupported INT 1Ah function"});
    return int1a_result{.ticks = timer.ticks(), .midnight_rollovers = timer.take_midnight_rollovers()};
}

} // namespace dosrecomp::runtime
