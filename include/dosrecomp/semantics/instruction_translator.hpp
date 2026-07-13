/** @file instruction_translator.hpp @brief Proven 8086 instruction effects lowered into register SSA. */
#pragma once

#include "dosrecomp/decoder/effective_address.hpp"
#include "dosrecomp/decoder/instruction_decoder.hpp"
#include "dosrecomp/ir/register_ssa.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace dosrecomp::runtime { class real_mode_memory; }

namespace dosrecomp::semantics {

/** A single MOV/CMP/... access into real-mode physical memory. */
struct memory_access {
    bool is_write{};
    std::uint32_t physical_address{};
    decoder::operand_width width{decoder::operand_width::word};
};

/** A concrete effect emitted by the semantic translation boundary. */
struct semantic_effect {
    ir::register_id destination{};
    std::size_t ssa_value{};
    std::optional<std::uint16_t> immediate;
    std::optional<memory_access> memory;
};

/** A semantically unsupported or inconsistent decoded instruction. */
struct translation_error { std::string message; };

/**
 * Translates verified instruction encodings into SSA definitions.
 *
 * When @p registers and @p memory are supplied, MOV reg↔mem operands are
 * lowered against the provided concrete state; the corresponding memory
 * access is reported in @ref semantic_effect::memory.
 */
class instruction_translator final {
public:
    [[nodiscard]] static std::expected<semantic_effect, translation_error>
    translate(const std::vector<std::byte>& code, const decoder::instruction& instruction,
              ir::register_ssa_builder& ssa, ir::register_state& state,
              decoder::register_values* registers = nullptr,
              runtime::real_mode_memory* memory = nullptr,
              std::uint16_t default_segment = 0);
};

} // namespace dosrecomp::semantics
