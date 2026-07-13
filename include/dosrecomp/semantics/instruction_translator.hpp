/** @file instruction_translator.hpp @brief Proven 8086 instruction effects lowered into register SSA. */
#pragma once

#include "dosrecomp/decoder/instruction_decoder.hpp"
#include "dosrecomp/ir/register_ssa.hpp"

#include <cstddef>
#include <expected>
#include <string>
#include <vector>

namespace dosrecomp::semantics {

/** A concrete effect emitted by the semantic translation boundary. */
struct semantic_effect {
    ir::register_id destination{};
    std::size_t ssa_value{};
    std::uint16_t immediate{};
};

/** A semantically unsupported or inconsistent decoded instruction. */
struct translation_error { std::string message; };

/** Translates verified instruction encodings into SSA definitions. */
class instruction_translator final {
public:
    [[nodiscard]] static std::expected<semantic_effect, translation_error>
    translate(const std::vector<std::byte>& code, const decoder::instruction& instruction,
              ir::register_ssa_builder& ssa, ir::register_state& state);
};

} // namespace dosrecomp::semantics

