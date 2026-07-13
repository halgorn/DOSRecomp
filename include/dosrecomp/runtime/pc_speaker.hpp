/** @file pc_speaker.hpp @brief Deterministic PC Speaker tone state. */
#pragma once

#include <cstdint>
#include <expected>
#include <string>

namespace dosrecomp::runtime {

struct speaker_error { std::string message; };

/** Models the active PC Speaker tone independently of host audio backends. */
class pc_speaker final {
public:
    [[nodiscard]] std::expected<void, speaker_error> start(std::uint32_t frequency_hz);
    void stop() noexcept;
    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] std::uint32_t frequency_hz() const noexcept;

private:
    bool active_{};
    std::uint32_t frequency_hz_{};
};

} // namespace dosrecomp::runtime

