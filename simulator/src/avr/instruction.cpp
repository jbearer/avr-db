#include <bitset>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include "math.h"

#include "avr/instruction.h"

using namespace avr;

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

int32_t twos_compliment(uint32_t n, size_t num_bits)
{
    bool sign_bit = n >> (num_bits - 1);
    uint32_t sign_mask = sign_bit << (num_bits - 1);
    int32_t sign_offset = -1*pow(2, num_bits-1) * sign_bit;
    return sign_offset + (~sign_mask & n);
}

void avr::process_reg1_reg2(instruction& instr, std::map<char, uint16_t> fields)
{
    instr.size = 1;
    instr.args.register1_register2.register1 = fields['r'];
    instr.args.register1_register2.register2 = fields['d'];
}

void avr::process_nothing(instruction& instr, std::map<char, uint16_t>)
{
    instr.size = 1;
}

void avr::process_constant_reg(instruction& instr, std::map<char, uint16_t> fields)
{
    instr.size = 1;
    instr.args.constant_register.constant = fields['K'];
    instr.args.constant_register.reg = fields['d'] + 16;
}

void avr::process_offset(instruction& instr, std::map<char, uint16_t> fields)
{
    instr.size = 1;
    instr.args.offset.offset = twos_compliment(fields['u'], 7);
}

void avr::process_offset12(instruction& instr, std::map<char, uint16_t> fields)
{
    instr.size = 1;
    instr.args.offset12.offset = twos_compliment(fields['u'], 12);
}

void avr::process_ioaddress_reg(instruction& instr, std::map<char, uint16_t> fields)
{
    instr.size = 1;
    instr.args.ioaddress_register.ioaddress = fields['a'];
    instr.args.ioaddress_register.reg = fields['d'];
}

void avr::process_constant_reg_pair(instruction& instr, std::map<char, uint16_t> fields)
{
    instr.size = 1;
    instr.args.constant_register_pair.pair = to_reg_pair(fields['p']);
    instr.args.constant_register_pair.constant = fields['k'];
}

void avr::process_reg(instruction& instr, std::map<char, uint16_t> fields)
{
    instr.size = 1;
    instr.args.reg.reg = fields['d'];
}

void avr::process_reg_address(instruction& instr, std::map<char, uint16_t> fields)
{
    instr.size = 2;
    instr.args.reg_address.reg = fields['d'];
}

void avr::process_address(instruction& instr, std::map<char, uint16_t> fields)
{
    instr.size = 2;
    instr.args.address.address = fields['k'];
}

instruction avr::decode(const uint16_t *pc)
{
    std::underlying_type_t<opcode> instr = *pc;

    for (auto& expr : avr::expressions) {
        std::unique_ptr<instruction> match = expr.matches(instr);
        if (match) {

            // special cases for 32-bit instructions
            if (match->op == opcode::LDS || match->op == opcode::STS) {
                match->args.reg_address.address = *(pc + 1);
            }
            else if (match->op == opcode::JMP || match->op == opcode::CALL) {
                match->args.address.address <<= 16;
                match->args.address.address |= *(pc + 1);
            }

            return *match;
        }
    }

    throw invalid_instruction_error(pc);
}

avr::instr_exp::instr_exp(opcode name, const std::string& exp, process_func process) :
    name_{name}, process_{process}
{

    mask_ = 0;
    masked_opcode_ = 0;
    size_t index = 0;
    for (auto c : exp) {
        if (c == ' ') {
            continue;   // ignore spaces
        }
        if (c == '0') {
            mask_ |= 1;
            masked_opcode_ |= 0;
        }
        else if (c == '1') {
            mask_ |= 1;
            masked_opcode_ |= 1;
        }
        else {
            if (fields_.count(c) == 0) {
                fields_[c] = {index};
            }
            else {
                fields_[c].push_back(index);
            }
        }
        ++index;
        if (index == sizeof(mask_)*8)    // don't want to shift the masks one last time
            break;

        mask_ <<= 1;
        masked_opcode_ <<= 1;
    }
}

std::unique_ptr<instruction> avr::instr_exp::matches(const uint16_t instr)
{
    if ((instr & mask_) == masked_opcode_) {
        std::map<char, uint16_t> bit_fields;
        for (auto& p : fields_) {
            bit_fields.emplace(p.first, bits_at(instr, p.second));
        }

        instruction instr;
        bzero(&instr, sizeof(instr));
        instr.op = name_;
        process_(instr, bit_fields);
        return std::make_unique<instruction>(instr);
    }
    else {
        return nullptr;
    }
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
