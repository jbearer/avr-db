#include "gtest/gtest.h"

#include "avr/instruction.h"

#include "decode.h"

using namespace avr;
using namespace testing;

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
    auto instr = decode_raw<32>(0b1001010'00000'111'0'0000111111110000);

    ASSERT_EQ(opcode::CALL, instr.op);
    ASSERT_EQ(4, instr.size);
    EXPECT_EQ(0x0FF0, instr.args.address.address);
}

TEST(decode, jmp)
{
    //                            opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    auto instr = decode_raw<32>(0b1001010'00000'110'0'0000111111110000);

    ASSERT_EQ(opcode::JMP, instr.op);
    ASSERT_EQ(4, instr.size);
    EXPECT_EQ(0x0FF0, instr.args.address.address);
}

TEST(decode, sts)
{
    //                            oooo ooo rrrrr oooo kkkkkkkkkkkkkkkk
    auto instr = decode_raw<32>(0b1001'001'10101'0000'1010101010101010);

    ASSERT_EQ(opcode::STS, instr.op);
    ASSERT_EQ(4, instr.size);
    EXPECT_EQ(0b10101, instr.args.reg_address.reg);
    EXPECT_EQ(0b1010101010101010, instr.args.reg_address.address);
}

TEST(decode, ret)
{
    // just the opcode
    auto instr = decode_raw<16>(0b1001'0101'0000'1000);
    ASSERT_EQ(opcode::RET, instr.op);
    ASSERT_EQ(2, instr.size);
}

TEST(decode, cp)
{   //                            oooo oo r ddddd rrrr
    auto instr = decode_raw<16>(0b0000'01'0'10101'1100);
    ASSERT_EQ(opcode::CP, instr.op);
    ASSERT_EQ(2, instr.size);
    EXPECT_EQ(0b01100, instr.args.register1_register2.register1);
    EXPECT_EQ(0b10101, instr.args.register1_register2.register2);
}
