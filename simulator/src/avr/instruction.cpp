#include <bitset>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include "math.h"

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

invalid_instruction_error::invalid_instruction_error(const uint16_t *pc)
    : desc("invalid instruction: ")
{
    desc += std::bitset<8>(*pc).to_string() + ' '
          + std::bitset<8>(*pc >> 8).to_string() + ' '
          + std::bitset<8>(*(pc + 1)).to_string() + ' '
          + std::bitset<8>(*(pc + 1) >> 8).to_string();
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
    uint16_t mask = 0;
    for (size_t i = 0; i < range; ++i) {
        mask <<= 1;
        mask |= 1;
    }
    return (bits >> (16 - max)) & mask;
}

instruction avr::decode(const uint16_t *pc)
{
    instruction instr;
    bzero(&instr, sizeof(instr));

    // 16-bit contiguous opcodes
    std::underlying_type_t<opcode> opcode16 = *pc;
    switch(opcode16) {
    case opcode::RET:
        instr.op = to_opcode(opcode16);
        instr.size = 1;
        return instr;
    }

    // 8-bit contiguous opcodes
    std::underlying_type_t<opcode> opcode8 = *pc & 0xFF00;
    switch (opcode8) {
    case opcode::ADIW:
    case opcode::SBIW:
        instr.op = to_opcode(opcode8);
        instr.size = 1;
        instr.args.constant_register_pair.pair = to_reg_pair(bits_range(*pc, 10, 12));
        instr.args.constant_register_pair.constant = bits_at(*pc, std::vector<size_t>{8,9,12,13,14,15});
        return instr;

    // Not an 8-bit opcode, try to match the next opcode type
    }

    // contiguous 6-bit opcodes for cp, cpc, sub, subc, etc.
    std::underlying_type_t<opcode> opcode6 = *pc & 0xFC00;
    switch (opcode6) {
    case opcode::CP:
    case opcode::CPC:
    case opcode::ADD:
    case opcode::ADC:
    case opcode::EOR:
        instr.op = to_opcode(opcode6);
        instr.size = 1;
        instr.args.register1_register2.register1 = bits_at(*pc, std::vector<size_t>{6,12,13,14,15});
        instr.args.register1_register2.register2 = bits_range(*pc, 7, 12);
        return instr;
    }

    // contiguous 4-bit opcode
    std::underlying_type_t<opcode> opcode4 = *pc & 0xF000;
    switch (opcode4) {
    case opcode::LDI:
    case opcode::CPI:
        instr.op = to_opcode(opcode4);
        instr.size = 1;
        instr.args.constant_register.constant = bits_at(*pc, std::vector<size_t>{4,5,6,7,12,13,14,15});
        instr.args.constant_register.reg = bits_range(*pc, 8, 12) + 16;
        return instr;
    case opcode::RJMP:
    case opcode::RCALL:
        {
            instr.op = to_opcode(opcode4);
            instr.size = 1;
            uint16_t signed_offset = bits_range(*pc, 4, 16);
            bool sign_bit = signed_offset >> 11;
            uint16_t sign_mask = sign_bit << 11;
            if (sign_bit)
                instr.args.offset12.offset = (~sign_mask & signed_offset) + -1*pow(2,11);
            else
                instr.args.offset12.offset = signed_offset;
        }
        return instr;
    }

    // discontiguous 9-bit branch opcodes
    std::underlying_type_t<opcode> opcode9 = *pc & 0b1111'1100'0000'0111;
    switch (opcode9) {
    case opcode::BRGE:
    case opcode::BRNE:
        {
            instr.op = to_opcode(opcode9);
            instr.size = 1;
            uint8_t signed_offset = bits_range(*pc, 6, 13);
            bool sign_bit = signed_offset >> 6;
            uint8_t sign_mask = sign_bit << 6;
            if (sign_bit)
                instr.args.offset.offset = (~sign_mask & signed_offset) + -1*pow(2,6);
            else
                instr.args.offset.offset = signed_offset;
        }
        return instr;
    }

    // contiguous 5-bit opcode
    std::underlying_type_t<opcode> opcode5 = *pc & 0xF800;
    switch (opcode5) {
    case opcode::IN:
    case opcode::OUT:
        instr.op = to_opcode(opcode5);
        instr.size = 1;
        instr.args.ioaddress_register.ioaddress = bits_at(*pc, std::vector<size_t>{5,6,12,13,14,15});
        instr.args.ioaddress_register.reg = bits_range(*pc, 7, 12);
        return instr;
    }

    // 10-bit discontiguous opcodes (CALL and JMP)
    std::underlying_type_t<opcode> opcode10 = *pc & 0b1111'1110'0000'1110;
    switch (opcode10) {
    case opcode::CALL:
    case opcode::JMP:
        // TODO assumes address is at most 16 bits (true on ATmega168, not on all AVR boards)
        instr.op = to_opcode(opcode10);
        instr.size = 2;
        instr.args.address.address = *(pc + 1);
        return instr;

    // Not a 10-bit discontiguous opcode
    }

    // 11-bit discontinuous opcodes (LDS and STS)
    std::underlying_type_t<opcode> opcode11 = *pc & 0b1111'1110'0000'1111;
    switch (opcode11) {
    case opcode::STS:
    case opcode::LDS:
        instr.op = to_opcode(opcode11);
        instr.size = 2;
        instr.args.reg_address.reg = bits_range(*pc, 7, 12);
        instr.args.reg_address.address = *(pc + 1);
        return instr;
    case opcode::LPM:
    case opcode::STX:
    case opcode::PUSH:
    case opcode::POP:
        instr.op = to_opcode(opcode11);
        instr.size = 1;
        instr.args.reg.reg = bits_range(*pc, 7, 12);
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
    case STS:
        return "sts";
    case RET:
        return "ret";
    case CP:
        return "cp";
    case CPC:
        return "cpc";
    case ADD:
        return "add";
    case ADC:
        return "adc";
    case LDI:
        return "ldi";
    case LDS:
        return "lds";
    case RJMP:
        return "rjmp";
    case RCALL:
        return "rcall";
    case EOR:
        return "eor";
    case IN:
        return "in";
    case OUT:
        return "out";
    case BRNE:
        return "brne";
    case CPI:
        return "cpi";
    case STX:
        return "stx";
    case LPM:
        return "lpm";
    case PUSH:
        return "push";
    case POP:
        return "pop";
    default:
        throw invalid_instruction_error(instr);
    }
}
