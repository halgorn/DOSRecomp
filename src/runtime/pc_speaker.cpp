#include "dosrecomp/runtime/pc_speaker.hpp"

namespace dosrecomp::runtime {
std::expected<void, speaker_error> pc_speaker::start(std::uint32_t frequency_hz) {
    if (frequency_hz < 19 || frequency_hz > 1'193'182) {
        return std::unexpected(speaker_error{"PC Speaker frequency is outside PIT range"});
    }
    frequency_hz_ = frequency_hz;
    active_ = true;
    return {};
}

void pc_speaker::stop() noexcept { active_ = false; frequency_hz_ = 0; }
bool pc_speaker::active() const noexcept { return active_; }
std::uint32_t pc_speaker::frequency_hz() const noexcept { return frequency_hz_; }

} // namespace dosrecomp::runtime

