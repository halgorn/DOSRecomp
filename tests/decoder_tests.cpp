#include "dosrecomp/decoder/instruction_decoder.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
std::byte b(unsigned value) { return static_cast<std::byte>(value); }
bool expect(bool condition, std::string_view description) {
    if (!condition) std::cerr << "FAILED: " << description << '\n';
    return condition;
}
}

int main() {
    using namespace dosrecomp::decoder;
    const std::vector<std::byte> code{b(0xb8), b(0x34), b(0x12), b(0x74), b(0xfc), b(0xe8), b(0x10), b(0x00)};
    const auto move = instruction_decoder::decode_at(code, 0);
    const auto branch = instruction_decoder::decode_at(code, 3);
    const auto call = instruction_decoder::decode_at(code, 5);
    const auto truncated = instruction_decoder::decode_at({b(0xe8)}, 0);
    const auto unknown = instruction_decoder::decode_at({b(0xff)}, 0);
    const auto modrm = instruction_decoder::decode_at({b(0x89), b(0x86), b(0x34), b(0x12)}, 0);
    const auto arithmetic = instruction_decoder::decode_at({b(0x83), b(0x6e), b(0xfe), b(0x01)}, 0);
    const bool passed = expect(move && move->size == 3 && move->kind == instruction_kind::move_immediate, "decode MOV immediate") &&
        expect(branch && branch->kind == instruction_kind::conditional_jump && branch->relative_target == -4, "decode conditional branch") &&
        expect(call && call->kind == instruction_kind::call && call->relative_target == 16, "decode relative call") &&
        expect(!truncated, "reject truncated instruction") && expect(!unknown, "reject unknown opcode");
    return (passed && expect(modrm && modrm->kind == instruction_kind::move && modrm->size == 4, "decode ModR/M displacement") &&
        expect(arithmetic && arithmetic->kind == instruction_kind::arithmetic && arithmetic->size == 4, "decode arithmetic immediate")) ? EXIT_SUCCESS : EXIT_FAILURE;
}
