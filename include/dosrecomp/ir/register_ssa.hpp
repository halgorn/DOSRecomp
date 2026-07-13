/** @file register_ssa.hpp @brief SSA versioning for the 8086 architectural registers. */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dosrecomp::ir {

/** Registers modeled by the initial real-mode SSA layer. */
enum class register_id : std::uint8_t { ax, bx, cx, dx, si, di, bp, sp, flags, count };

/** How an SSA value was introduced. */
enum class value_kind : std::uint8_t { entry, definition, phi };

/** Immutable SSA definition and its incoming value IDs. */
struct ssa_value {
    std::size_t id{};
    register_id reg{};
    value_kind kind{};
    std::vector<std::size_t> inputs;
};

/** A register-to-current-SSA-value mapping at one program point. */
struct register_state {
    std::array<std::size_t, static_cast<std::size_t>(register_id::count)> values{};
};

/**
 * Creates SSA values and merges register states.
 *
 * The caller owns CFG traversal; this class only enforces immutable versions
 * and creates phi values where two predecessor versions differ.
 */
class register_ssa_builder final {
public:
    register_ssa_builder();

    [[nodiscard]] register_state entry_state() const noexcept;
    [[nodiscard]] std::size_t define(register_state& state, register_id reg, std::vector<std::size_t> inputs = {});
    [[nodiscard]] register_state merge(const register_state& first, const register_state& second);
    [[nodiscard]] const std::vector<ssa_value>& values() const noexcept;

private:
    std::vector<ssa_value> values_;
};

} // namespace dosrecomp::ir

