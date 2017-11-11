#pragma once

#include <exception>
#include <string>

#include "types.h"

namespace avr {

    enum opcode
        : uint16_t
    {
        ADIW = 0b1001'0110'0000'0000,
        SBIW = 0b1001'0111'0000'0000,
        CALL = 0b1001'0100'0000'1110,
        JMP  = 0b1001'0100'0000'1100,
        STS  = 0b1001'0010'0000'0000,
        RET  = 0b1001'0101'0000'1000,
        CP   = 0b0001'0100'0000'0000,
        CPC  = 0b0000'0100'0000'0000,
        ROL  = 0b0001'1100'0000'0000,
        LSL  = 0b0000'1100'0000'0000,
        LDI  = 0b1110'0000'0000'0000,
        CPI  = 0b0011'0000'0000'0000,
        LDS  = 0b1001'0000'0000'0000,
        STX  = 0b1001'0010'0000'1101,
        BRGE = 0b1111'0100'0000'0100,
        BRNE = 0b1111'0100'0000'0001,
        RJMP = 0b1100'0000'0000'0000,
        EOR  = 0b0010'0100'0000'0000,
        OUT  = 0b1011'1000'0000'0000,
        LPM  = 0b1001'0000'0000'0100
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
