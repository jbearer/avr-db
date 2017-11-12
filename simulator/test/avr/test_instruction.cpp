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
    ASSERT_EQ(1, instr.size);
    EXPECT_EQ(Y, instr.args.constant_register_pair.pair);
    EXPECT_EQ(0b0001'0110, instr.args.constant_register_pair.constant);
}

TEST(decode, sbiw)
{
    //                            opop'opop'kk'pp'kkkk
    auto instr = decode_raw<16>(0b1001'0111'01'01'0110);

    ASSERT_EQ(opcode::SBIW, instr.op);
    ASSERT_EQ(1, instr.size);
    EXPECT_EQ(X, instr.args.constant_register_pair.pair);
    EXPECT_EQ(0b0001'0110, instr.args.constant_register_pair.constant);
}

TEST(decode, call)
{
    //                            opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    auto instr = decode_raw<32>(0b1001010'00000'111'0'0000111111110000);

    ASSERT_EQ(opcode::CALL, instr.op);
    ASSERT_EQ(2, instr.size);
    EXPECT_EQ(0x0FF0, instr.args.address.address);
}

TEST(decode, jmp)
{
    //                            opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    auto instr = decode_raw<32>(0b1001010'00000'110'0'0000111111110000);

    ASSERT_EQ(opcode::JMP, instr.op);
    ASSERT_EQ(2, instr.size);
    EXPECT_EQ(0x0FF0, instr.args.address.address);
}

TEST(decode, sts)
{
    //                            oooo ooo rrrrr oooo kkkk kkkk kkkk kkkk
    auto instr = decode_raw<32>(0b1001'001'10101'0000'1010'1010'1010'1010);

    ASSERT_EQ(opcode::STS, instr.op);
    ASSERT_EQ(2, instr.size);
    EXPECT_EQ(0b10101, instr.args.reg_address.reg);
    EXPECT_EQ(0b1010'1010'1010'1010, instr.args.reg_address.address);
}

TEST(decode, lds)
{
    //                            oooo ooo rrrrr oooo kkkk kkkk kkkk kkkk
    auto instr = decode_raw<32>(0b1001'000'00101'0000'1010'1010'1010'1000);

    ASSERT_EQ(opcode::LDS, instr.op);
    ASSERT_EQ(2, instr.size);
    EXPECT_EQ(0b00101, instr.args.reg_address.reg);
    EXPECT_EQ(0b1010'1010'1010'1000, instr.args.reg_address.address);
}

TEST(decode, ret)
{
    // just the opcode
    auto instr = decode_raw<16>(0b1001'0101'0000'1000);
    ASSERT_EQ(opcode::RET, instr.op);
    ASSERT_EQ(1, instr.size);
}

TEST(decode, cp)
{   //                            oooo oo r ddddd rrrr
    auto instr = decode_raw<16>(0b0001'01'0'10101'1100);
    ASSERT_EQ(opcode::CP, instr.op);
    ASSERT_EQ(1, instr.size);
    EXPECT_EQ(0b01100, instr.args.register1_register2.register1);
    EXPECT_EQ(0b10101, instr.args.register1_register2.register2);
}

TEST(decode, cpc)
{
    auto instr = decode_raw<16>(0b0000'01'0'01010'0110);
    ASSERT_EQ(opcode::CPC, instr.op);
    ASSERT_EQ(1, instr.size);
    EXPECT_EQ(0b00110, instr.args.register1_register2.register1);
    EXPECT_EQ(0b01010, instr.args.register1_register2.register2);
}

TEST(decode, add)
{
    auto instr = decode_raw<16>(0b0000'11'0'01010'0110);
    ASSERT_EQ(opcode::ADD, instr.op);
    ASSERT_EQ(1, instr.size);
    EXPECT_EQ(0b00110, instr.args.register1_register2.register1);
    EXPECT_EQ(0b01010, instr.args.register1_register2.register2);
}

TEST(decode, adc)
{
    auto instr = decode_raw<16>(0b0001'11'0'01010'0110);
    ASSERT_EQ(opcode::ADC, instr.op);
    ASSERT_EQ(1, instr.size);
    EXPECT_EQ(0b00110, instr.args.register1_register2.register1);
    EXPECT_EQ(0b01010, instr.args.register1_register2.register2);
}

TEST(decode, ldi)
{
    //                            oooo KKKK dddd KKKK
    auto instr = decode_raw<16>(0b1110'0110'1001'1010);
    ASSERT_EQ(opcode::LDI, instr.op);
    ASSERT_EQ(1, instr.size);
    EXPECT_EQ(0b0110'1010, instr.args.constant_register.constant);
    EXPECT_EQ(0b1001 + 16, instr.args.constant_register.reg);
}

TEST(decode, out)
{
    //                            ooooo aa ddddd aaaa
    auto instr = decode_raw<16>(0b10111'01'10101'0011);
    ASSERT_EQ(opcode::OUT, instr.op);
    ASSERT_EQ(1, instr.size);
    EXPECT_EQ(0b01'0011, instr.args.ioaddress_register.ioaddress);
    EXPECT_EQ(0b10101, instr.args.ioaddress_register.reg);
}


TEST(decode, brge)
{   //                            oooo oo kkkkkkk ooo
    auto instr = decode_raw<16>(0b1111'01'0011011'100);
    ASSERT_EQ(opcode::BRGE, instr.op);
    ASSERT_EQ(1, instr.size);
    EXPECT_EQ(0b0011011, instr.args.offset.offset);

    auto instr2 = decode_raw<16>(0b1111'01'1111000'100);
    ASSERT_EQ(opcode::BRGE, instr2.op);
    ASSERT_EQ(1, instr2.size);
    EXPECT_EQ(-8, instr2.args.offset.offset);
}

TEST(decode, rjmp)
{

    auto instr = decode_raw<16>(0b1100'0001'1001'0110);
    ASSERT_EQ(opcode::RJMP, instr.op);
    ASSERT_EQ(1, instr.size);
    EXPECT_EQ(0b0001'1001'0110, instr.args.offset12.offset);

    // test signed
    auto instr2 = decode_raw<16>(0b1100'1111'1111'1101);
    ASSERT_EQ(opcode::RJMP, instr2.op);
    ASSERT_EQ(1, instr2.size);
    EXPECT_EQ(-3, instr2.args.offset12.offset);

}

TEST(bit_helpers, test1)
{
    uint16_t bits = 0b0101'0000'0000'0000;
    EXPECT_EQ(0b110, avr::bits_at(bits, std::vector<size_t>{1,3,5}));
}

TEST(bit_helpers, test2)
{
    uint16_t bits = 0b0110'0101'1010'1100;
    EXPECT_EQ(0b01100, avr::bits_range(bits, 0, 5));
    EXPECT_EQ(0b0101, avr::bits_range(bits, 4, 8));
    EXPECT_EQ(0b100, avr::bits_range(bits, 13, 16));
}

TEST(decode, eor)
{
    // eor r3,r5                  oooo oo r ddddd rrrr
    auto instr = decode_raw<16>(0b0010'01'0'00101'0011);
    ASSERT_EQ(opcode::EOR, instr.op);
    ASSERT_EQ(1, instr.size);
    EXPECT_EQ(0b00011, instr.args.register1_register2.register1);
    EXPECT_EQ(0b00101, instr.args.register1_register2.register2);
}
