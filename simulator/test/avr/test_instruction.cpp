#include "gtest/gtest.h"

#include "avr/instruction.h"

using namespace avr;

template<size_t>
struct uint_n;

template<>
struct uint_n<16>
{
    using type = uint16_t;
};

template<>
struct uint_n<32>
{
    using type = uint32_t;
};

template<size_t n> using uint_n_t = typename uint_n<n>::type;

template<size_t n> instruction decode_raw(uint_n_t<n>);

template<>
instruction decode_raw<16>(uint16_t raw)
{
    byte_t *bytes = reinterpret_cast<byte_t *>(&raw);
    byte_t pc[2];

    // Convert from little-endian
    pc[0] = bytes[1];
    pc[1] = bytes[0];

    return decode(pc);
}

template<>
instruction decode_raw<32>(uint32_t raw)
{
    byte_t *bytes = reinterpret_cast<byte_t *>(&raw);
    byte_t pc[4];

    // Convert from little-endian
    pc[0] = bytes[3];
    pc[1] = bytes[2];
    pc[2] = bytes[1];
    pc[3] = bytes[0];

    return decode(pc);
}

TEST(decode, adiw)
{
    //                            opop'opop'kk'pp'kkkk
    auto instr = decode_raw<16>(0b1001'0110'01'10'0110);

    ASSERT_EQ(opcode::ADIW, instr.op);
    ASSERT_EQ(2, instr.size);
    EXPECT_EQ(Y, instr.args.constant_register_pair.pair);
    EXPECT_EQ(0b0001'0110, instr.args.constant_register_pair.constant);
}

TEST(decode, sbiw)
{
    //                            opop'opop'kk'pp'kkkk
    auto instr = decode_raw<16>(0b1001'0111'01'01'0110);

    ASSERT_EQ(opcode::SBIW, instr.op);
    ASSERT_EQ(2, instr.size);
    EXPECT_EQ(X, instr.args.constant_register_pair.pair);
    EXPECT_EQ(0b0001'0110, instr.args.constant_register_pair.constant);
}

TEST(decode, call)
{
    //                            opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    auto instr = decode_raw<32>(0b1001010'01010'111'1'0101010101010101);

    ASSERT_EQ(opcode::CALL, instr.op);
    ASSERT_EQ(4, instr.size);
    EXPECT_EQ(0b010101'01010101'01010101, instr.args.address.address);
}

TEST(decode, jmp)
{
    //                            opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    auto instr = decode_raw<32>(0b1001010'01010'110'1'0101010101010101);

    ASSERT_EQ(opcode::JMP, instr.op);
    ASSERT_EQ(4, instr.size);
    EXPECT_EQ(0b010101'01010101'01010101, instr.args.address.address);
}

TEST(decode, sts)
{

    auto instr = decode_raw<32>(0b1001'001'10101'0000'1010101010101010);

    ASSERT_EQ(opcode::STS, instr.op);
    ASSERT_EQ(4, instr.size);
    EXPECT_EQ(0b10101, instr.args.reg_address.reg);
    EXPECT_EQ(0b1010101010101010, instr.args.reg_address.address);
}

