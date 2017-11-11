#pragma once

#include <exception>
#include <string>

#include "types.h"

namespace avr {

    enum opcode
        : uint16_t
    {
        ADIW = 0b1001'0110,
        SBIW = 0b1001'0111,
        CALL = 0b10'0101'0111,
        JMP = 0b10'0101'0110,
        STS = 0b10'0100'1'0000,
        RET = 0b1001'0101'0000'1000,
        CP  = 0b0000'01,
        LDI = 0b1110
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
        } args;

        bool operator==(const instruction &) const;
        bool operator!=(const instruction &) const;
    };

    instruction decode(const byte_t *pc);

    // bits are read LEFT TO RIGHT, indexed at 0
    uint16_t bits_at(uint16_t bits, const std::vector<size_t>& locations);
    // returns bits in range [min, max)
    uint16_t bits_range(uint16_t bits, size_t min, size_t max);

    std::string mnemonic(const instruction &);

    struct invalid_instruction_error
        : std::exception
    {
        invalid_instruction_error(const byte_t *pc);
        invalid_instruction_error(const instruction &);
        const char *what() const noexcept override;

    private:
        std::string desc;
    };
}
