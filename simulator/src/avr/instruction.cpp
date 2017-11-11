#include <bitset>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>

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

invalid_instruction_error::invalid_instruction_error(const byte_t *pc)
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

bool instruction::operator==(const instruction & that) const
{
    return std::memcmp(this, &that, sizeof(*this)) == 0; // TODO ewwwwwwww
}

bool instruction::operator!=(const instruction & that) const
{
    return !(*this == that);
}

address_t avr::register_pair_address(register_pair pair)
{
    return 24 + 2*pair;
}

<<<<<<< 74bdb8f990e08ce44b9231b89c72af18bcc3b0c9
instruction avr::decode(const byte_t *pc)
=======
uint16_t avr::bits_at(uint16_t bits, const std::vector<size_t>& locations)
{
    uint16_t new_bits = 0;
    for (auto location : locations) {
        uint16_t new_bit = (bits >> (15 - location)) & 1;
        new_bits = (new_bits << 1) | new_bit;
    }
    return new_bits;
}

uint16_t avr::bits_range(uint16_t bits, size_t min, size_t max)
{
    size_t range = max - min;
    uint16_t mask;
    for (size_t i = 0; i < range; ++i) {
        mask <<= 1;
        mask |= 1;
    }
    return (bits >> (16 - max)) & mask;
}

instruction avr::decode(byte_t *pc)
>>>>>>> added ldi and helper methods
{
    instruction instr;

    uint16_t word1 = (*pc << 8)| *(pc + 1);

    // 16-bit contiguous opcodes
    std::underlying_type_t<opcode> opcode16 = (*pc << 8)| *(pc + 1);
    switch(opcode16) {
    case opcode::RET:
        instr.op = to_opcode(opcode16);
        instr.size = 2;
        return instr;
    }

    // 8-bit contiguous opcodes
    std::underlying_type_t<opcode> opcode8 = bits_range(word1, 0, 8);
    switch (opcode8) {
    case opcode::ADIW:
    case opcode::SBIW:
        instr.op = to_opcode(opcode8);
        instr.size = 2;
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
        {
            // TODO assumes address is at most 16 bits (true on ATmega168, not on all AVR boards)
            instr.op = to_opcode(opcode10);
            instr.size = 4;
            byte_t *addr_bytes = reinterpret_cast<byte_t *>(&instr.args.address.address);
            addr_bytes[0] = *(pc + 3);
            addr_bytes[1] = *(pc + 2);
            return instr;
        }

    // Not a 10-bit discontiguous opcode
    }

    // 11-bit discontinuous opcodes (LDS and STS)
    std::underlying_type_t<opcode> opcode11 = (*pc >> 1) << 4;  // left 7 bits
    opcode11 |= (*(pc + 1) & 0x0F);                             // right 4 bits
    switch (opcode11) {
    case opcode::STS:
        instr.op = to_opcode(opcode11);
        instr.size = 4;
        instr.args.reg_address.reg = (*pc & 0x1) << 4; // left bit of reg
        instr.args.reg_address.reg |= (*(pc + 1) & 0xF0) >> 4; // right 4 bits of reg
        instr.args.reg_address.address = *(reinterpret_cast<const uint16_t *>(pc + 2));
        return instr;
    }

    // discontiguous 6-bit opcodes for cp, cpc, sub, subc, etc.
    std::underlying_type_t<opcode> opcode6 = (*pc >> 2) & 0b0011'1111;
    switch (opcode6) {
    case opcode::CP:
        instr.op = to_opcode(opcode6);
        instr.size = 2;
        instr.args.register1_register2.register1 = ((*pc & 0b0010) << 3) | (*(pc + 1) & 0xF);
        instr.args.register1_register2.register2 = ((*pc & 0b0001) << 4) | ((*(pc + 1) & 0XF0) >> 4);
        return instr;
        }

    // contiguous 4-bit opcode
    std::underlying_type_t<opcode> opcode4 = bits_range(word1, 0, 4);
    switch (opcode4) {
    case opcode::LDI:
        instr.op = to_opcode(opcode4);
        instr.size = 2;
        instr.args.constant_register.constant = bits_at(word1, std::vector<size_t>{4,5,6,7,12,13,14,15});
        instr.args.constant_register.reg = bits_range(word1, 8, 12) + 16;
        return instr;
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
