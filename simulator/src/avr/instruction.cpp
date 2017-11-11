#include <bitset>
#include <string>

#include "avr/instruction.h"

using namespace avr;

static opcode to_opcode(std::underlying_type_t<opcode> raw)
{
    return static_cast<opcode>(raw);
}

static register_pair to_reg_pair(std::underlying_type_t<register_pair> raw)
{
    return static_cast<register_pair>(raw);
}

invalid_instruction_error::invalid_instruction_error(byte_t *pc)
    : desc("invalid instruction: ")
{
    desc += std::bitset<8>(*pc).to_string() + ' '
          + std::bitset<8>(*(pc + 1)).to_string() + ' '
          + std::bitset<8>(*(pc + 2)).to_string() + ' '
          + std::bitset<8>(*(pc + 3)).to_string();
}

invalid_instruction_error::invalid_instruction_error(const instruction & instr)
    : desc("invalid opcode: ")
{
    desc += instr.op;
}

const char *invalid_instruction_error::what() const noexcept
{
    return desc.c_str();
}

address_t avr::register_pair_address(register_pair pair)
{
    return 24 + 2*pair;
}

instruction avr::decode(byte_t *pc)
{
    instruction instr;

    // 8-bit contiguous opcodes
    std::underlying_type_t<opcode> opcode8 = *pc;
    switch (opcode8) {
    case opcode::ADIW:
    case opcode::SBIW:
        instr.op = to_opcode(opcode8);
        instr.args.constant_register_pair.pair = to_reg_pair((*(pc + 1) & 0b0011'0000) >> 4);
        instr.args.constant_register_pair.constant = (*(pc + 1)) & 0b0000'1111;
        instr.args.constant_register_pair.constant |= (*(pc + 1) & 0b1100'0000) >> 2;
        return instr;

    // Not an 8-bit opcode, try to match the next opcode type
    }

    // 10-bit discontiguous opcodes (CALL and JMP)
    std::underlying_type_t<opcode> opcode10 = (*(pc + 1) & 0b1110) >> 1;
    opcode10 |= (*pc & 0b1111'1110) << 2;
    switch (opcode10) {
    case opcode::CALL:
    case opcode::JMP:
        instr.op = to_opcode(opcode10);
        instr.args.address.address = *(reinterpret_cast<uint16_t *>(pc) + 1);
        instr.args.address.address |= (*(pc + 1) & 0b0000'0001) << 16;
        instr.args.address.address |= (*(pc + 1) & 0b1111'0000) << (17-4);
        instr.args.address.address |= (*pc & 1) << 21;
        return instr;

    // Not a 10-bit discontiguous opcode
    }

    throw invalid_instruction_error(pc);
}

std::string avr::mnemonic(const instruction & instr)
{
    switch (instr.op) {
    case ADIW:
        return "adiw";
    case SBIW:
        return "sbiw";
    case CALL:
        return "call";
    case JMP:
        return "jmp";
    default:
        throw invalid_instruction_error(instr);
    }
}
