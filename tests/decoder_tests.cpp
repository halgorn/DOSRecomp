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
    const auto register_modrm = instruction_decoder::decode_at({b(0x8b), b(0xd8)}, 0);
    const auto arithmetic = instruction_decoder::decode_at({b(0x83), b(0x6e), b(0xfe), b(0x01)}, 0);
    const auto accumulator = instruction_decoder::decode_at({b(0x3d), b(0x00), b(0x00)}, 0);
    const auto flag = instruction_decoder::decode_at({b(0xfd)}, 0);
    const auto string = instruction_decoder::decode_at({b(0xa5)}, 0);
    const auto io = instruction_decoder::decode_at({b(0xe6), b(0x20)}, 0);
    const auto interrupt3 = instruction_decoder::decode_at({b(0xcc)}, 0);
    const auto far_return = instruction_decoder::decode_at({b(0xcb)}, 0);
    const auto exchange = instruction_decoder::decode_at({b(0x93)}, 0);
    const auto translate = instruction_decoder::decode_at({b(0xd7)}, 0);
    const auto moffs = instruction_decoder::decode_at({b(0xa1), b(0x34), b(0x12)}, 0);
    const auto sign_extend = instruction_decoder::decode_at({b(0x99)}, 0);
    const auto adjust = instruction_decoder::decode_at({b(0xd4), b(10)}, 0);
    const auto test_group = instruction_decoder::decode_at({b(0xf7), b(0x06), b(0x34), b(0x12), b(0xff), b(0xff)}, 0);
    const auto increment = instruction_decoder::decode_at({b(0xfe), b(0x46), b(0xfe)}, 0);
    const auto decimal_adjust = instruction_decoder::decode_at({b(0x27)}, 0);
    const auto shift = instruction_decoder::decode_at({b(0xd1), b(0x66), b(0xfe)}, 0);
    const auto repeated_string = instruction_decoder::decode_at({b(0xf3), b(0xa4)}, 0);
    const auto segment_move = instruction_decoder::decode_at({b(0x26), b(0x8b), b(0x07)}, 0);
    const auto repeated_segment_string = instruction_decoder::decode_at({b(0xf3), b(0x26), b(0xa4)}, 0);
    const auto far_pointer_load = instruction_decoder::decode_at({b(0xc4), b(0x06), b(0x34), b(0x12)}, 0);
    const auto coprocessor = instruction_decoder::decode_at({b(0xd8), b(0x06), b(0x34), b(0x12)}, 0);
    const auto byte_move = instruction_decoder::decode_at({b(0xb4), b(0x4c)}, 0);
    const bool passed = expect(move && move->size == 3 && move->kind == instruction_kind::move_immediate && move->operand_count == 2 &&
        move->operands[0].kind == operand_kind::reg && move->operands[0].reg == register_name::ax &&
        move->operands[1].immediate == 0x1234, "decode MOV immediate operands") &&
        expect(byte_move && byte_move->operands[0].reg == register_name::ah && byte_move->operands[1].immediate == 0x4c, "decode byte MOV operands") &&
        expect(branch && branch->kind == instruction_kind::conditional_jump && branch->relative_target == -4 && branch->condition == branch_condition::equal, "decode conditional branch") &&
        expect(call && call->kind == instruction_kind::call && call->relative_target == 16, "decode relative call") &&
        expect(!truncated, "reject truncated instruction") && expect(!unknown, "reject unknown opcode");
    return (passed && expect(modrm && modrm->kind == instruction_kind::move && modrm->size == 4, "decode ModR/M displacement") &&
        expect(modrm && modrm->operand_count == 2 && modrm->operands[0].kind == operand_kind::memory && modrm->operands[0].address_registers[0] == register_name::bp && modrm->operands[0].displacement == 0x1234 && modrm->operands[1].reg == register_name::ax, "decode memory MOV operands") &&
        expect(register_modrm && register_modrm->operands[0].reg == register_name::bx && register_modrm->operands[1].reg == register_name::ax, "decode register ModR/M MOV operands") &&
        expect(arithmetic && arithmetic->kind == instruction_kind::arithmetic && arithmetic->size == 4, "decode arithmetic immediate") &&
        expect(accumulator && accumulator->kind == instruction_kind::compare && accumulator->size == 3 && accumulator->operand_count == 2 &&
            accumulator->operands[0].reg == register_name::ax && accumulator->operands[1].immediate == 0 && accumulator->alu == alu_operation::compare, "decode accumulator immediate") &&
        expect(flag && flag->kind == instruction_kind::flags, "decode flag control") &&
        expect(string && string->kind == instruction_kind::string, "decode string instruction") &&
        expect(io && io->kind == instruction_kind::io && io->size == 2, "decode port I/O") &&
        expect(interrupt3 && interrupt3->interrupt_number == 3, "decode INT 3") &&
        expect(far_return && far_return->kind == instruction_kind::return_, "decode far return") &&
        expect(exchange && exchange->kind == instruction_kind::move, "decode AX register exchange") &&
        expect(translate && translate->kind == instruction_kind::move, "decode XLAT") &&
        expect(moffs && moffs->kind == instruction_kind::move && moffs->size == 3, "decode moffs move") &&
        expect(sign_extend && sign_extend->kind == instruction_kind::arithmetic, "decode CWD") &&
        expect(adjust && adjust->kind == instruction_kind::arithmetic && adjust->size == 2, "decode AAM") &&
        expect(test_group && test_group->kind == instruction_kind::compare && test_group->size == 6, "decode F7 test group") &&
        expect(increment && increment->kind == instruction_kind::arithmetic && increment->size == 3, "decode FE increment group") &&
        expect(decimal_adjust && decimal_adjust->kind == instruction_kind::arithmetic, "decode decimal adjustment") &&
        expect(shift && shift->kind == instruction_kind::arithmetic && shift->size == 3, "decode shift group") &&
        expect(repeated_string && repeated_string->kind == instruction_kind::string && repeated_string->size == 2, "decode REP string") &&
        expect(segment_move && segment_move->kind == instruction_kind::move && segment_move->size == 3, "decode segment override") &&
        expect(repeated_segment_string && repeated_segment_string->kind == instruction_kind::string && repeated_segment_string->size == 3, "decode combined prefixes") &&
        expect(far_pointer_load && far_pointer_load->kind == instruction_kind::move && far_pointer_load->size == 4, "decode far pointer load") &&
        expect(coprocessor && coprocessor->kind == instruction_kind::coprocessor && coprocessor->size == 4, "decode coprocessor escape")) ? EXIT_SUCCESS : EXIT_FAILURE;
}
