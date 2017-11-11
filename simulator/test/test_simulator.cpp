#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "avr/instruction.h"
#include "avr/register.h"
#include "segment.h"
#include "simulator.h"

#include "decode.h"

using namespace avr;
using namespace simulator;
using namespace testing;

struct mock_segment
    : segment
{
    mock_segment(size_t size_, address_t address_, std::vector<byte_t> data_)
        : _size(size_)
        , _address(address_)
        , _data(std::move(data_))
    {}

    size_t size() const override
    {
        return _size;
    }

    address_t address() const override
    {
        return _address;
    }

    const byte_t *data() const override
    {
        return _data.data();
    }

private:
    size_t _size;
    address_t _address;
    std::vector<byte_t> _data;
};

void instr_to_bytes(std::vector<byte_t> & v, uint32_t instr)
{
    byte_t *bytes = reinterpret_cast<byte_t *>(&instr);
    v.push_back(bytes[3]);
    v.push_back(bytes[2]);
    v.push_back(bytes[1]);
    v.push_back(bytes[0]);
}

void instr_to_bytes(std::vector<byte_t> & v, uint16_t instr)
{
    byte_t *bytes = reinterpret_cast<byte_t *>(&instr);
    v.push_back(bytes[1]);
    v.push_back(bytes[0]);
}

std::unique_ptr<segment> empty_segment()
{
    return std::make_unique<mock_segment>(0, 0, std::vector<byte_t>());
}

std::unique_ptr<segment> text_segment(const std::vector<byte_t> & bytes)
{
    auto size = bytes.size();
    return std::make_unique<mock_segment>(size, 0, std::move(bytes));
}

uint16_t stack_pointer(const simulator::simulator & sim)
{
    uint16_t sp;
    byte_t *bytes = reinterpret_cast<byte_t *>(&sp);
    bytes[0] = sim.read(SPL);
    bytes[1] = sim.read(SPH);
    return sp;
}

TEST(adiw, sum)
{
    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t instr =    0b1001'0110'01'01'0110;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, instr);
    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());
    sim->step();

    byte_t lo_x = sim->read(X_LO);
    byte_t hi_x = sim->read(X_HI);

    EXPECT_EQ(22, lo_x);
    EXPECT_EQ(0,  hi_x);
}

TEST(sbiw, sum)
{
    // sbiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t instr =    0b1001'0111'01'01'0110;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, instr);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());
    sim->step();

    int16_t x;
    byte_t *bytes = reinterpret_cast<byte_t *>(&x);
    bytes[0] = sim->read(X_LO);
    bytes[1] = sim->read(X_HI);

    EXPECT_EQ(-22, x);
}

TEST(call, call)
{
    // call 6         opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    uint32_t call = 0b1001010'00000'111'0'0000000000000110;

    uint16_t unreachable = 0b1001'0111'01'01'0110;

    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t add =      0b1001'0110'01'01'0110;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, call);
    instr_to_bytes(text_bytes, unreachable);
    instr_to_bytes(text_bytes, add);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    auto sp1 = stack_pointer(*sim);
    sim->step();
    auto sp2 = stack_pointer(*sim);

    EXPECT_EQ(decode_raw<16>(add), sim->next_instruction());
    EXPECT_EQ(sp1 - 2, sp2);
    EXPECT_EQ(4, sim->read(sp1));
    EXPECT_EQ(0, sim->read(sp1 + 1));
}

TEST(ret, after_call)
{
    // call 6         opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    uint32_t call = 0b1001010'00000'111'0'0000000000000110;

    // sbiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t sub =      0b1001'0111'01'01'0110;

    // ret
    uint16_t ret = 0b1001'0101'0000'1000;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, call);
    instr_to_bytes(text_bytes, sub);
    instr_to_bytes(text_bytes, ret);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();

    EXPECT_EQ(decode_raw<16>(sub), sim->next_instruction());
}

TEST(jmp, jmp)
{
    // jmp 6         opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    uint32_t jmp = 0b1001010'00000'110'0'0000000000000110;

    uint16_t unreachable = 0b1001'0111'01'01'0110;

    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t add =      0b1001'0110'01'01'0110;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, jmp);
    instr_to_bytes(text_bytes, unreachable);
    instr_to_bytes(text_bytes, add);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    EXPECT_EQ(decode_raw<16>(add), sim->next_instruction());
}

TEST(sts, sts)
{
    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t add =      0b1001'0110'01'01'0110;

    // sts r26,500   oooo ooo rrrrr oooo kkkkkkkkkkkkkkkk
    uint32_t sts = 0b1001'001'11010'0000'0000000111110100;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, add);
    instr_to_bytes(text_bytes, sts);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    EXPECT_EQ(22, sim->read(0b0000000111110100));
}

TEST(rol, rol)
{
    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t add =      0b1001'0110'01'01'0110;

    // rol r26       opop'op'r'ddddd'rrrr
    uint16_t rol = 0b0001'11'1'11010'1010;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, add);
    instr_to_bytes(text_bytes, rol);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    EXPECT_EQ(0b0101100, sim->read(26));
}

TEST(lsl, lsl)
{
    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t add =      0b1001'0110'01'01'0110;

    // lsl r26       opop'op'r'ddddd'rrrr
    uint16_t lsl = 0b0000'11'1'11010'1010;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, add);
    instr_to_bytes(text_bytes, lsl);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    EXPECT_EQ(0b0101100, sim->read(26));
}

TEST(ldi, ldi)
{
    // ldi r25,0x6A    oooo KKKK dddd KKKK
    uint16_t instr = 0b1110'0110'1001'1010;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, instr);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();

    EXPECT_EQ(0x6A, sim->read(25));
}

TEST(lds, lds)
{
    // ldi r25,0x6A    oooo KKKK dddd KKKK
    uint16_t ldi = 0b1110'0110'1001'1010;

    // lds r10,25    oooo ooo rrrrr oooo kkkk kkkk kkkk kkkk
    uint32_t lds = 0b1001'000'01010'0000'0000'0000'0001'1001;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi);
    instr_to_bytes(text_bytes, lds);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();

    EXPECT_EQ(0x6A, sim->read(25));
    EXPECT_EQ(0x6A, sim->read(10));
}

TEST(brge, do_branch)
{
    // ldi r16,1       oooo KKKK dddd KKKK
    uint16_t ldi16 = 0b1110'0000'0000'0001;

    // ldi r17,2       oooo KKKK dddd KKKK
    uint16_t ldi17 = 0b1110'0000'0001'0010;

    // r17,r16      oooo oo r ddddd rrrr
    uint16_t cp = 0b0001'01'1'10001'0000;

    // brge 2         oooo oo kkkkkkk ooo
    uint16_t brge = 0b1111'01'0000010'100;

    // sbiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t sub =      0b1001'0111'01'01'0110;

    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t add =      0b1001'0110'01'01'0110;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi16);
    instr_to_bytes(text_bytes, ldi17);
    instr_to_bytes(text_bytes, cp);
    instr_to_bytes(text_bytes, brge);
    instr_to_bytes(text_bytes, sub);
    instr_to_bytes(text_bytes, add);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(decode_raw<16>(add), sim->next_instruction());
}

TEST(brge, dont_branch)
{
    // ldi r16,1       oooo KKKK dddd KKKK
    uint16_t ldi16 = 0b1110'0000'0000'0001;

    // ldi r17,2       oooo KKKK dddd KKKK
    uint16_t ldi17 = 0b1110'0000'0001'0010;

    // r16,r17      oooo oo r ddddd rrrr
    uint16_t cp = 0b0001'01'1'10000'0001;

    // brge 2         oooo oo kkkkkkk ooo
    uint16_t brge = 0b1111'01'0000010'100;

    // sbiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t sub =      0b1001'0111'01'01'0110;

    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t add =      0b1001'0110'01'01'0110;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi16);
    instr_to_bytes(text_bytes, ldi17);
    instr_to_bytes(text_bytes, cp);
    instr_to_bytes(text_bytes, brge);
    instr_to_bytes(text_bytes, sub);
    instr_to_bytes(text_bytes, add);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(decode_raw<16>(sub), sim->next_instruction());
}
