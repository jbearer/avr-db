#pragma once

#include <exception>
#include <string>
#include <map>
#include <vector>
#include <memory>

#include "types.h"

namespace avr {

    enum opcode
    {
        ADIW,
        SBIW,
        CALL,
        JMP,
        STS,
        RET,
        CP,
        CPC,
        ADD,
        ADC,
        LDI,
        CPI,
        LDS,
        STX,
        BRGE,
        BRNE,
        RJMP,
        EOR,
        IN,
        OUT,
        LPM,
        RCALL,
        PUSH,
        POP
    };

    enum register_pair
    {
        W = 0b00,
        X = 0b01,
        Y = 0b10,
        Z = 0b11
    };

    address_t register_pair_address(register_pair);

    struct constant_register_pair_args
    {
        uint8_t         constant;
        register_pair   pair;
    };

    struct address_args
    {
        address_t       address;
    };

    struct register_address_args
    {
        uint8_t         reg;
        address_t       address;
    };

    struct register1_register2_args
    {
        bool            carry;
        uint8_t         register1;  // r register
        uint8_t         register2;  // d register
    };

    struct constant_register_args
    {
        uint8_t         constant;
        uint8_t         reg;
    };

    struct offset_args
    {
        int8_t          offset;
    };

    struct offset12_args
    {
        int16_t         offset;
    };

    struct ioaddress_register_args
    {
        uint8_t         ioaddress;
        uint8_t         reg;
    };

    struct register_args
    {
        uint8_t         reg;
    };

    struct instruction
    {
        opcode          op;
        uint8_t         size;
        union {
            constant_register_pair_args constant_register_pair;
            address_args                address;
            register_address_args       reg_address;
            register1_register2_args    register1_register2;
            constant_register_args      constant_register;
            offset_args                 offset;
            offset12_args               offset12;
            ioaddress_register_args     ioaddress_register;
            register_args               reg;
        } args;

        bool operator==(const instruction &) const;
        bool operator!=(const instruction &) const;
    };

    instruction decode(const uint16_t *pc);

    // bits are read LEFT TO RIGHT, indexed at 0
    uint16_t bits_at(uint16_t bits, const std::vector<size_t>& locations);
    // returns bits in range [min, max)
    uint16_t bits_range(uint16_t bits, size_t min, size_t max);

    std::string mnemonic(const instruction &);

    typedef void (*process_func)(instruction&, std::map<char, uint16_t>);

    // processing functions for decoding instructions
    void process_nothing(instruction& instr, std::map<char, uint16_t> fields);
    void process_reg1_reg2(instruction& instr, std::map<char, uint16_t> fields);
    void process_constant_reg(instruction& instr, std::map<char, uint16_t> fields);
    void process_offset(instruction& instr, std::map<char, uint16_t> fields);
    void process_offset12(instruction& instr, std::map<char, uint16_t> fields);
    void process_ioaddress_reg(instruction& instr, std::map<char, uint16_t> fields);
    void process_constant_reg_pair(instruction& instr, std::map<char, uint16_t> fields);
    void process_reg(instruction& instr, std::map<char, uint16_t> fields);
    void process_reg_address(instruction& instr, std::map<char, uint16_t> fields);
    void process_address(instruction& instr, std::map<char, uint16_t> fields);

    // Matches an against instruction bits
    class instr_exp {
    public:
        instr_exp(opcode name, const std::string& exp, process_func process);
        std::unique_ptr<instruction> matches(const uint16_t instr);

    private:
        opcode          name_;
        uint16_t        mask_;
        uint16_t        masked_opcode_;
        process_func    process_;
        std::map<char, std::vector<size_t>>   fields_;
    };

    static std::vector<instr_exp> expressions
    {
        {opcode::RET,  "1001 0101 0000 1000", &process_nothing},
        {opcode::ADD,  "0000 11r ddddd rrrr", &process_reg1_reg2},
        {opcode::ADC,  "0001 11r ddddd rrrr", &process_reg1_reg2},
        {opcode::CP,   "0001 01r ddddd rrrr", &process_reg1_reg2},
        {opcode::CPC,  "0000 01r ddddd rrrr", &process_reg1_reg2},
        {opcode::EOR,  "0010 01r ddddd rrrr", &process_reg1_reg2},
        {opcode::LDI,  "1110 KKKK dddd KKKK", &process_constant_reg},
        {opcode::CPI,  "0011 KKKK dddd KKKK", &process_constant_reg},
        {opcode::BRGE, "1111 01uu uuuu u100", &process_offset},
        {opcode::BRNE, "1111 01uu uuuu u001", &process_offset},
        {opcode::RJMP, "1100 uuuu uuuu uuuu", &process_offset12},
        {opcode::RCALL,"1101 uuuu uuuu uuuu", &process_offset12},
        {opcode::IN,   "1011 0aa ddddd aaaa", &process_ioaddress_reg},
        {opcode::OUT,  "1011 1aa ddddd aaaa", &process_ioaddress_reg},
        {opcode::ADIW, "1001 0110 kkpp kkkk", &process_constant_reg_pair},
        {opcode::SBIW, "1001 0111 kkpp kkkk", &process_constant_reg_pair},
        {opcode::PUSH, "1001 001 ddddd 1111", &process_reg},
        {opcode::POP,  "1001 000 ddddd 1111", &process_reg},
        {opcode::STX,  "1001 001 ddddd 1101", &process_reg},
        {opcode::LPM,  "1001 000 ddddd 0101", &process_reg},
        // 32-bit instructions:
        {opcode::STS,  "1001 001 ddddd 0000", &process_reg_address},
        {opcode::LDS,  "1001 000 ddddd 0000", &process_reg_address},
        {opcode::CALL, "1001 010 kkkkk 111k", &process_address},
        {opcode::JMP,  "1001 010 kkkkk 110k", &process_address}
    };

    struct invalid_instruction_error
        : std::exception
    {
        invalid_instruction_error(const uint16_t *pc);
        invalid_instruction_error(const instruction &);
        const char *what() const noexcept override;

    private:
        std::string desc;
    };

}
