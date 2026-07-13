#include "dosrecomp/semantics/instruction_translator.hpp"

#include "dosrecomp/runtime/real_mode_memory.hpp"

#include <cstdlib>
#include <iostream>

namespace { std::byte b(unsigned value) { return static_cast<std::byte>(value); } }

int main() {
    const std::vector<std::byte> code{b(0xb8), b(0x34), b(0x12)};
    const auto decoded = dosrecomp::decoder::instruction_decoder::decode_at(code, 0);
    if (!decoded) return EXIT_FAILURE;
    dosrecomp::ir::register_ssa_builder builder;
    auto state = builder.entry_state();
    const auto effect = dosrecomp::semantics::instruction_translator::translate(code, *decoded, builder, state);
    const std::vector<std::byte> register_code{b(0x8b), b(0xc3)};
    const auto register_decoded = dosrecomp::decoder::instruction_decoder::decode_at(register_code, 0);
    const auto register_effect = register_decoded ? dosrecomp::semantics::instruction_translator::translate(register_code, *register_decoded, builder, state)
                                                  : std::expected<dosrecomp::semantics::semantic_effect, dosrecomp::semantics::translation_error>{std::unexpected(dosrecomp::semantics::translation_error{"decode failed"})};
    const std::vector<std::byte> add_code{b(0x05), b(1), b(0)};
    const auto add_decoded = dosrecomp::decoder::instruction_decoder::decode_at(add_code, 0);
    const auto add_effect = add_decoded ? dosrecomp::semantics::instruction_translator::translate(add_code, *add_decoded, builder, state)
                                        : std::expected<dosrecomp::semantics::semantic_effect, dosrecomp::semantics::translation_error>{std::unexpected(dosrecomp::semantics::translation_error{"decode failed"})};
    const std::vector<std::byte> compare_code{b(0x3d), b(0), b(0)};
    const auto compare_decoded = dosrecomp::decoder::instruction_decoder::decode_at(compare_code, 0);
    const auto compare_effect = compare_decoded ? dosrecomp::semantics::instruction_translator::translate(compare_code, *compare_decoded, builder, state)
                                                : std::expected<dosrecomp::semantics::semantic_effect, dosrecomp::semantics::translation_error>{std::unexpected(dosrecomp::semantics::translation_error{"decode failed"})};
    const std::vector<std::byte> test_code{b(0xa9), b(1), b(0)};
    const auto test_decoded = dosrecomp::decoder::instruction_decoder::decode_at(test_code, 0);
    const auto test_effect = test_decoded ? dosrecomp::semantics::instruction_translator::translate(test_code, *test_decoded, builder, state)
                                          : std::expected<dosrecomp::semantics::semantic_effect, dosrecomp::semantics::translation_error>{std::unexpected(dosrecomp::semantics::translation_error{"decode failed"})};
    if (!effect || effect->destination != dosrecomp::ir::register_id::ax || effect->immediate != 0x1234 || !register_effect || register_effect->immediate || !add_effect || !compare_effect || !test_effect ||
        compare_effect->destination != dosrecomp::ir::register_id::flags || builder.values()[add_effect->ssa_value].operation != dosrecomp::ir::operation_kind::add ||
        builder.values()[compare_effect->ssa_value].operation != dosrecomp::ir::operation_kind::compare ||
        builder.values()[test_effect->ssa_value].operation != dosrecomp::ir::operation_kind::test || builder.values().size() != 25) {
        std::cerr << "failed to translate MOV AX, imm16\n";
        return EXIT_FAILURE;
    }
    const auto unsupported = dosrecomp::semantics::instruction_translator::translate({b(0x90)},
        {dosrecomp::decoder::instruction_kind::nop, 0, 1, 0, 0}, builder, state);
    if (unsupported) {
        std::cerr << "failed to reject unsupported instruction\n";
        return EXIT_FAILURE;
    }

    dosrecomp::runtime::real_mode_memory memory;
    if (const auto seeded = memory.write16(0x0020, 0xbeef); !seeded) return EXIT_FAILURE;
    dosrecomp::decoder::register_values registers{};
    registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::bx)] = 0x0020;

    const std::vector<std::byte> load_code{b(0x8b), b(0x07)};
    const auto load_decoded = dosrecomp::decoder::instruction_decoder::decode_at(load_code, 0);
    if (!load_decoded || load_decoded->kind != dosrecomp::decoder::instruction_kind::move || load_decoded->operand_count != 2 ||
        load_decoded->operands[0].kind != dosrecomp::decoder::operand_kind::reg ||
        load_decoded->operands[1].kind != dosrecomp::decoder::operand_kind::memory) {
        std::cerr << "failed to decode MOV AX, [BX]\n";
        return EXIT_FAILURE;
    }
    const auto load_effect = dosrecomp::semantics::instruction_translator::translate(load_code, *load_decoded, builder, state, &registers, &memory);
    if (!load_effect || load_effect->destination != dosrecomp::ir::register_id::ax || load_effect->immediate != 0xbeef ||
        !load_effect->memory || load_effect->memory->is_write || load_effect->memory->physical_address != 0x20 ||
        builder.values()[load_effect->ssa_value].constant != 0xbeef) {
        std::cerr << "failed to translate MOV AX, [BX]\n";
        return EXIT_FAILURE;
    }

    registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::ax)] = 0x1234;
    const std::vector<std::byte> store_code{b(0x89), b(0x07)};
    const auto store_decoded = dosrecomp::decoder::instruction_decoder::decode_at(store_code, 0);
    if (!store_decoded || store_decoded->kind != dosrecomp::decoder::instruction_kind::move || store_decoded->operand_count != 2 ||
        store_decoded->operands[0].kind != dosrecomp::decoder::operand_kind::memory ||
        store_decoded->operands[1].kind != dosrecomp::decoder::operand_kind::reg) {
        std::cerr << "failed to decode MOV [BX], AX\n";
        return EXIT_FAILURE;
    }
    const auto store_effect = dosrecomp::semantics::instruction_translator::translate(store_code, *store_decoded, builder, state, &registers, &memory);
    if (!store_effect || !store_effect->memory || !store_effect->memory->is_write ||
        store_effect->memory->physical_address != 0x20) {
        std::cerr << "failed to translate MOV [BX], AX\n";
        return EXIT_FAILURE;
    }
    const auto stored = memory.read16(0x0020);
    if (!stored || *stored != 0x1234) {
        std::cerr << "failed to write MOV [BX], AX through real_mode_memory\n";
        return EXIT_FAILURE;
    }

    const auto missing_context = dosrecomp::semantics::instruction_translator::translate(load_code, *load_decoded, builder, state);
    if (missing_context) {
        std::cerr << "failed to reject MOV reg,mem without register and memory context\n";
        return EXIT_FAILURE;
    }

    dosrecomp::ir::register_ssa_builder reg_builder;
    auto reg_state = reg_builder.entry_state();
    const std::vector<std::byte> setup_code{b(0xb8), b(0x05), b(0x00), b(0xbb), b(0x03), b(0x00)};
    std::size_t pos = 0;
    while (pos < setup_code.size()) {
        const auto d = dosrecomp::decoder::instruction_decoder::decode_at(setup_code, pos);
        if (!d) { std::cerr << "reg-reg setup decode failed at " << pos << "\n"; return EXIT_FAILURE; }
        const auto e = dosrecomp::semantics::instruction_translator::translate(setup_code, *d, reg_builder, reg_state);
        if (!e) { std::cerr << "reg-reg setup translate failed at " << pos << ": " << e.error().message << "\n"; return EXIT_FAILURE; }
        pos += d->size;
    }
    const std::vector<std::byte> add_ax_bx{b(0x01), b(0xd8)};
    const auto add_rr_decoded = dosrecomp::decoder::instruction_decoder::decode_at(add_ax_bx, 0);
    if (!add_rr_decoded || add_rr_decoded->kind != dosrecomp::decoder::instruction_kind::arithmetic || add_rr_decoded->operand_count != 2 ||
        add_rr_decoded->operands[0].kind != dosrecomp::decoder::operand_kind::reg ||
        add_rr_decoded->operands[1].kind != dosrecomp::decoder::operand_kind::reg ||
        add_rr_decoded->operands[0].reg != dosrecomp::decoder::register_name::ax ||
        add_rr_decoded->operands[1].reg != dosrecomp::decoder::register_name::bx ||
        add_rr_decoded->alu != dosrecomp::decoder::alu_operation::add) {
        std::cerr << "failed to decode ADD AX, BX (modrm 01 d8)\n";
        return EXIT_FAILURE;
    }
    const auto add_rr_effect = dosrecomp::semantics::instruction_translator::translate(add_ax_bx, *add_rr_decoded, reg_builder, reg_state);
    if (!add_rr_effect || add_rr_effect->destination != dosrecomp::ir::register_id::ax ||
        reg_builder.values()[add_rr_effect->ssa_value].operation != dosrecomp::ir::operation_kind::add) {
        std::cerr << "failed to translate ADD AX, BX\n";
        return EXIT_FAILURE;
    }
    const std::vector<std::byte> sub_bx_ax{b(0x29), b(0xc3)};
    const auto sub_decoded = dosrecomp::decoder::instruction_decoder::decode_at(sub_bx_ax, 0);
    if (!sub_decoded || sub_decoded->kind != dosrecomp::decoder::instruction_kind::arithmetic ||
        sub_decoded->operands[0].reg != dosrecomp::decoder::register_name::bx ||
        sub_decoded->operands[1].reg != dosrecomp::decoder::register_name::ax ||
        sub_decoded->alu != dosrecomp::decoder::alu_operation::subtract) {
        std::cerr << "failed to decode SUB BX, AX (modrm 29 c3)\n";
        return EXIT_FAILURE;
    }
    const auto sub_rr_effect = dosrecomp::semantics::instruction_translator::translate(sub_bx_ax, *sub_decoded, reg_builder, reg_state);
    if (!sub_rr_effect || sub_rr_effect->destination != dosrecomp::ir::register_id::bx ||
        reg_builder.values()[sub_rr_effect->ssa_value].operation != dosrecomp::ir::operation_kind::subtract) {
        std::cerr << "failed to translate SUB BX, AX\n";
        return EXIT_FAILURE;
    }
    const std::vector<std::byte> cmp_bx_ax{b(0x39), b(0xc3)};
    const auto cmp_decoded = dosrecomp::decoder::instruction_decoder::decode_at(cmp_bx_ax, 0);
    if (!cmp_decoded || cmp_decoded->kind != dosrecomp::decoder::instruction_kind::compare ||
        cmp_decoded->alu != dosrecomp::decoder::alu_operation::compare) {
        std::cerr << "failed to decode CMP BX, AX (modrm 39 c3)\n";
        return EXIT_FAILURE;
    }
    const auto cmp_rr_effect = dosrecomp::semantics::instruction_translator::translate(cmp_bx_ax, *cmp_decoded, reg_builder, reg_state);
    if (!cmp_rr_effect || cmp_rr_effect->destination != dosrecomp::ir::register_id::flags ||
        reg_builder.values()[cmp_rr_effect->ssa_value].operation != dosrecomp::ir::operation_kind::compare) {
        std::cerr << "failed to translate CMP BX, AX\n";
        return EXIT_FAILURE;
    }

    const std::vector<std::byte> inc_cx_code{b(0x41)};
    const auto inc_decoded = dosrecomp::decoder::instruction_decoder::decode_at(inc_cx_code, 0);
    if (!inc_decoded || inc_decoded->kind != dosrecomp::decoder::instruction_kind::arithmetic ||
        inc_decoded->operands[0].reg != dosrecomp::decoder::register_name::cx ||
        inc_decoded->operands[1].immediate != 1 || inc_decoded->alu != dosrecomp::decoder::alu_operation::add) {
        std::cerr << "failed to decode INC CX\n";
        return EXIT_FAILURE;
    }
    const auto inc_effect = dosrecomp::semantics::instruction_translator::translate(inc_cx_code, *inc_decoded, reg_builder, reg_state);
    if (!inc_effect || inc_effect->destination != dosrecomp::ir::register_id::cx ||
        reg_builder.values()[inc_effect->ssa_value].operation != dosrecomp::ir::operation_kind::add) {
        std::cerr << "failed to translate INC CX\n";
        return EXIT_FAILURE;
    }
    const std::vector<std::byte> dec_dx_code{b(0x4a)};
    const auto dec_decoded = dosrecomp::decoder::instruction_decoder::decode_at(dec_dx_code, 0);
    if (!dec_decoded || dec_decoded->kind != dosrecomp::decoder::instruction_kind::arithmetic ||
        dec_decoded->operands[0].reg != dosrecomp::decoder::register_name::dx ||
        dec_decoded->operands[1].immediate != 1 || dec_decoded->alu != dosrecomp::decoder::alu_operation::subtract) {
        std::cerr << "failed to decode DEC DX\n";
        return EXIT_FAILURE;
    }
    const auto dec_effect = dosrecomp::semantics::instruction_translator::translate(dec_dx_code, *dec_decoded, reg_builder, reg_state);
    if (!dec_effect || dec_effect->destination != dosrecomp::ir::register_id::dx ||
        reg_builder.values()[dec_effect->ssa_value].operation != dosrecomp::ir::operation_kind::subtract) {
        std::cerr << "failed to translate DEC DX\n";
        return EXIT_FAILURE;
    }
    const std::vector<std::byte> byte_alu_code{b(0x04), b(0x05)};
    const auto byte_alu_decoded = dosrecomp::decoder::instruction_decoder::decode_at(byte_alu_code, 0);
    if (!byte_alu_decoded || byte_alu_decoded->kind != dosrecomp::decoder::instruction_kind::arithmetic ||
        byte_alu_decoded->operands[0].reg != dosrecomp::decoder::register_name::al ||
        byte_alu_decoded->operands[0].width != dosrecomp::decoder::operand_width::byte ||
        byte_alu_decoded->operands[1].immediate != 5 ||
        byte_alu_decoded->alu != dosrecomp::decoder::alu_operation::add) {
        std::cerr << "failed to decode ADD AL, 5\n";
        return EXIT_FAILURE;
    }
    dosrecomp::ir::register_ssa_builder byte_alu_builder;
    auto byte_alu_state = byte_alu_builder.entry_state();
    const auto byte_alu_effect = dosrecomp::semantics::instruction_translator::translate(byte_alu_code, *byte_alu_decoded, byte_alu_builder, byte_alu_state);
    if (!byte_alu_effect || byte_alu_effect->destination != dosrecomp::ir::register_id::al ||
        byte_alu_builder.values()[byte_alu_effect->ssa_value].operation != dosrecomp::ir::operation_kind::add) {
        std::cerr << "failed to translate ADD AL, 5\n";
        return EXIT_FAILURE;
    }
    const std::vector<std::byte> byte_cmp_code{b(0x3c), b(0x07)};
    const auto byte_cmp_decoded = dosrecomp::decoder::instruction_decoder::decode_at(byte_cmp_code, 0);
    if (!byte_cmp_decoded || byte_cmp_decoded->kind != dosrecomp::decoder::instruction_kind::compare ||
        byte_cmp_decoded->operands[0].reg != dosrecomp::decoder::register_name::al ||
        byte_cmp_decoded->alu != dosrecomp::decoder::alu_operation::compare) {
        std::cerr << "failed to decode CMP AL, 7\n";
        return EXIT_FAILURE;
    }
    const auto byte_cmp_effect = dosrecomp::semantics::instruction_translator::translate(byte_cmp_code, *byte_cmp_decoded, byte_alu_builder, byte_alu_state);
    if (!byte_cmp_effect || byte_cmp_effect->destination != dosrecomp::ir::register_id::flags ||
        byte_alu_builder.values()[byte_cmp_effect->ssa_value].operation != dosrecomp::ir::operation_kind::compare) {
        std::cerr << "failed to translate CMP AL, 7\n";
        return EXIT_FAILURE;
    }

    dosrecomp::runtime::real_mode_memory stack_memory;
    dosrecomp::decoder::register_values stack_registers{};
    stack_registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::ax)] = 0xbeef;
    stack_registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::sp)] = 0x0100;
    dosrecomp::ir::register_ssa_builder stack_builder;
    auto stack_state = stack_builder.entry_state();

    const std::vector<std::byte> push_code{b(0x50)};
    const auto push_decoded = dosrecomp::decoder::instruction_decoder::decode_at(push_code, 0);
    if (!push_decoded || push_decoded->kind != dosrecomp::decoder::instruction_kind::stack ||
        push_decoded->stack_pop || push_decoded->operands[0].reg != dosrecomp::decoder::register_name::ax) {
        std::cerr << "failed to decode PUSH AX\n";
        return EXIT_FAILURE;
    }
    const auto push_effect = dosrecomp::semantics::instruction_translator::translate(push_code, *push_decoded, stack_builder, stack_state, &stack_registers, &stack_memory);
    if (!push_effect || push_effect->destination != dosrecomp::ir::register_id::sp ||
        stack_registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::sp)] != 0x00fe ||
        !push_effect->memory || !push_effect->memory->is_write || push_effect->memory->physical_address != 0xfe) {
        std::cerr << "failed to translate PUSH AX\n";
        return EXIT_FAILURE;
    }
    if (const auto stored = stack_memory.read16(0x00fe); !stored || *stored != 0xbeef) {
        std::cerr << "PUSH AX did not store value at new SP\n";
        return EXIT_FAILURE;
    }
    const std::vector<std::byte> pop_code{b(0x58)};
    const auto pop_decoded = dosrecomp::decoder::instruction_decoder::decode_at(pop_code, 0);
    if (!pop_decoded || pop_decoded->kind != dosrecomp::decoder::instruction_kind::stack ||
        !pop_decoded->stack_pop || pop_decoded->operands[0].reg != dosrecomp::decoder::register_name::ax) {
        std::cerr << "failed to decode POP AX\n";
        return EXIT_FAILURE;
    }
    const auto pop_effect = dosrecomp::semantics::instruction_translator::translate(pop_code, *pop_decoded, stack_builder, stack_state, &stack_registers, &stack_memory);
    if (!pop_effect || pop_effect->destination != dosrecomp::ir::register_id::ax ||
        !pop_effect->immediate || *pop_effect->immediate != 0xbeef ||
        stack_registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::sp)] != 0x0100 ||
        stack_registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::ax)] != 0xbeef ||
        !pop_effect->memory || pop_effect->memory->is_write || pop_effect->memory->physical_address != 0xfe) {
        std::cerr << "failed to translate POP AX\n";
        return EXIT_FAILURE;
    }

    const auto missing_stack_context = dosrecomp::semantics::instruction_translator::translate(push_code, *push_decoded, stack_builder, stack_state);
    if (missing_stack_context) {
        std::cerr << "failed to reject PUSH without register and memory context\n";
        return EXIT_FAILURE;
    }

    dosrecomp::runtime::real_mode_memory call_memory;
    dosrecomp::decoder::register_values call_registers{};
    call_registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::sp)] = 0x0200;
    dosrecomp::ir::register_ssa_builder call_builder;
    auto call_state = call_builder.entry_state();
    const std::vector<std::byte> call_code{b(0xe8), b(0x10), b(0x00)};
    const auto call_decoded = dosrecomp::decoder::instruction_decoder::decode_at(call_code, 0);
    if (!call_decoded || call_decoded->kind != dosrecomp::decoder::instruction_kind::call ||
        call_decoded->size != 3 || call_decoded->relative_target != 0x10) {
        std::cerr << "failed to decode CALL +0x10\n";
        return EXIT_FAILURE;
    }
    const auto call_effect = dosrecomp::semantics::instruction_translator::translate(call_code, *call_decoded, call_builder, call_state, &call_registers, &call_memory);
    if (!call_effect || call_effect->destination != dosrecomp::ir::register_id::sp ||
        call_registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::sp)] != 0x01fe ||
        !call_effect->memory || !call_effect->memory->is_write || call_effect->memory->physical_address != 0x1fe) {
        std::cerr << "failed to translate CALL\n";
        return EXIT_FAILURE;
    }
    if (const auto ret_addr = call_memory.read16(0x01fe); !ret_addr || *ret_addr != 3) {
        std::cerr << "CALL did not store return address at new SP\n";
        return EXIT_FAILURE;
    }

    const std::vector<std::byte> ret_code{b(0xc3)};
    const auto ret_decoded = dosrecomp::decoder::instruction_decoder::decode_at(ret_code, 0);
    if (!ret_decoded || ret_decoded->kind != dosrecomp::decoder::instruction_kind::return_ || ret_decoded->size != 1) {
        std::cerr << "failed to decode RET\n";
        return EXIT_FAILURE;
    }
    const auto ret_effect = dosrecomp::semantics::instruction_translator::translate(ret_code, *ret_decoded, call_builder, call_state, &call_registers, &call_memory);
    if (!ret_effect || ret_effect->destination != dosrecomp::ir::register_id::sp ||
        call_registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::sp)] != 0x0200 ||
        !ret_effect->memory || ret_effect->memory->is_write || ret_effect->memory->physical_address != 0x1fe) {
        std::cerr << "failed to translate RET\n";
        return EXIT_FAILURE;
    }

    const std::vector<std::byte> ret_imm_code{b(0xc2), b(0x04), b(0x00)};
    const auto ret_imm_decoded = dosrecomp::decoder::instruction_decoder::decode_at(ret_imm_code, 0);
    if (!ret_imm_decoded || ret_imm_decoded->size != 3) {
        std::cerr << "failed to decode RET imm16\n";
        return EXIT_FAILURE;
    }
    call_registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::sp)] = 0x0200;
    const auto ret_imm_effect = dosrecomp::semantics::instruction_translator::translate(ret_imm_code, *ret_imm_decoded, call_builder, call_state, &call_registers, &call_memory);
    if (!ret_imm_effect || call_registers[static_cast<std::size_t>(dosrecomp::decoder::register_name::sp)] != 0x0206) {
        std::cerr << "failed to translate RET imm16\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}