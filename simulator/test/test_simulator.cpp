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

    const byte_t *bytes() const override
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
    v.push_back(bytes[2]);
    v.push_back(bytes[3]);
    v.push_back(bytes[0]);
    v.push_back(bytes[1]);
}

void instr_to_bytes(std::vector<byte_t> & v, uint16_t instr)
{
    byte_t *bytes = reinterpret_cast<byte_t *>(&instr);
    v.push_back(bytes[0]);
    v.push_back(bytes[1]);
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

TEST(adiw, sreg_unsigned_flags)
{
    // ldi X_LO,254     oooo KKKK dddd KKKK
    uint16_t ldi_lo = 0b1110'1111'1010'1110;

    // ldi X_HI,255     oooo KKKK dddd KKKK
    uint16_t ldi_hi = 0b1110'1111'1011'1111;

    // adiw X,1       oooo'oooo'kk'pp'kkkk
    uint16_t adiw = 0b1001'0110'00'01'0001;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi_lo);
    instr_to_bytes(text_bytes, ldi_hi);
    instr_to_bytes(text_bytes, adiw);
    instr_to_bytes(text_bytes, adiw);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(0, sim->read(SREG) & SREG_C);
    EXPECT_EQ(0, sim->read(SREG) & SREG_Z);
    EXPECT_EQ(SREG_N, sim->read(SREG) & SREG_N);
    EXPECT_EQ(255, sim->read(X_LO));
    EXPECT_EQ(255, sim->read(X_HI));

    sim->step();

    EXPECT_EQ(SREG_C, sim->read(SREG) & SREG_C);
    EXPECT_EQ(SREG_Z, sim->read(SREG) & SREG_Z);
    EXPECT_EQ(0, sim->read(SREG) & SREG_N);
    EXPECT_EQ(0, sim->read(X_LO));
    EXPECT_EQ(0, sim->read(X_HI));
}

TEST(adiw, sreg_signed_flags)
{
    // ldi X_LO,255     oooo KKKK dddd KKKK
    uint16_t ldi_lo = 0b1110'1111'1010'1111;

    // ldi X_HI,127     oooo KKKK dddd KKKK
    uint16_t ldi_hi = 0b1110'0111'1011'1111;

    // adiw X,1       oooo'oooo'kk'pp'kkkk
    uint16_t adiw = 0b1001'0110'00'01'0001;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi_lo);
    instr_to_bytes(text_bytes, ldi_hi);
    instr_to_bytes(text_bytes, adiw);
    instr_to_bytes(text_bytes, adiw);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(SREG_V, sim->read(SREG) & SREG_V);
    EXPECT_EQ(SREG_N, sim->read(SREG) & SREG_N);
    EXPECT_EQ(0, sim->read(SREG) & SREG_S);
    EXPECT_EQ(0, sim->read(X_LO));
    EXPECT_EQ(0x80, sim->read(X_HI));

    sim->step();

    EXPECT_EQ(0, sim->read(SREG) & SREG_V);
    EXPECT_EQ(SREG_N, sim->read(SREG) & SREG_N);
    EXPECT_EQ(SREG_S, sim->read(SREG) & SREG_S);
    EXPECT_EQ(1, sim->read(X_LO));
    EXPECT_EQ(0x80, sim->read(X_HI));
}

TEST(sbiw, difference)
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

TEST(sbiw, sreg_unsigned_flags)
{
    // ldi X_LO,1    oooo KKKK dddd KKKK
    uint16_t ldi = 0b1110'0000'1010'0001;

    // sbiw X,1       oooo'oooo'kk'pp'kkkk
    uint16_t sbiw = 0b1001'0111'00'01'0001;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi);
    instr_to_bytes(text_bytes, sbiw);
    instr_to_bytes(text_bytes, sbiw);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();

    EXPECT_EQ(0, sim->read(SREG) & SREG_C);
    EXPECT_EQ(0, sim->read(SREG) & SREG_N);
    EXPECT_EQ(SREG_Z, sim->read(SREG) & SREG_Z);
    EXPECT_EQ(0, sim->read(X_LO));
    EXPECT_EQ(0, sim->read(X_HI));

    sim->step();

    EXPECT_EQ(SREG_C, sim->read(SREG) & SREG_C);
    EXPECT_EQ(SREG_N, sim->read(SREG) & SREG_N);
    EXPECT_EQ(0, sim->read(SREG) & SREG_Z);
    EXPECT_EQ(0xFF, sim->read(X_LO));
    EXPECT_EQ(0xFF, sim->read(X_HI));
}

TEST(sbiw, sreg_signed_flags)
{
    // ldi X_HI,0x80   oooo KKKK dddd KKKK
    uint16_t ldi =   0b1110'1000'1011'0000;

    // sbiw X,1       oooo'oooo'kk'pp'kkkk
    uint16_t sbiw = 0b1001'0111'00'01'0001;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi);
    instr_to_bytes(text_bytes, sbiw);
    instr_to_bytes(text_bytes, sbiw);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();

    EXPECT_EQ(SREG_V, sim->read(SREG) & SREG_V);
    EXPECT_EQ(0, sim->read(SREG) & SREG_N);
    EXPECT_EQ(SREG_S, sim->read(SREG) & SREG_S);
    EXPECT_EQ(0xFF, sim->read(X_LO));
    EXPECT_EQ(0x7F, sim->read(X_HI));

    sim->step();

    EXPECT_EQ(0, sim->read(SREG) & SREG_V);
    EXPECT_EQ(0, sim->read(SREG) & SREG_N);
    EXPECT_EQ(0, sim->read(SREG) & SREG_S);
    EXPECT_EQ(0xFE, sim->read(X_LO));
    EXPECT_EQ(0x7F, sim->read(X_HI));
}

TEST(add, sum)
{
    // ldi r16,1        oooo KKKK dddd KKKK
    uint16_t ldi_rd = 0b1110'0000'0000'0001;

    // ldi r17,2        oooo KKKK dddd KKKK
    uint16_t ldi_rr = 0b1110'0000'0001'0010;

    // add r16,r17   opop'op'r'ddddd'rrrr
    uint16_t add = 0b0000'11'1'10000'0001;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi_rd);
    instr_to_bytes(text_bytes, ldi_rr);
    instr_to_bytes(text_bytes, add);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(3, sim->read(16));
    EXPECT_EQ(2, sim->read(17));
}

TEST(add, sreg_unsigned_flags)
{
    // ldi r16,254      oooo KKKK dddd KKKK
    uint16_t ldi_rd = 0b1110'1111'0000'1110;

    // ldi r17,1        oooo KKKK dddd KKKK
    uint16_t ldi_rr = 0b1110'0000'0001'0001;

    // add r16,r17   opop'op'r'ddddd'rrrr
    uint16_t add = 0b0000'11'1'10000'0001;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi_rd);
    instr_to_bytes(text_bytes, ldi_rr);
    instr_to_bytes(text_bytes, add);
    instr_to_bytes(text_bytes, add);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(0, sim->read(SREG) & SREG_C);
    EXPECT_EQ(0, sim->read(SREG) & SREG_Z);
    EXPECT_EQ(SREG_N, sim->read(SREG) & SREG_N);
    EXPECT_EQ(0, sim->read(SREG) & SREG_H);
    EXPECT_EQ(255, sim->read(16));
    EXPECT_EQ(1, sim->read(17));

    sim->step();

    EXPECT_EQ(SREG_C, sim->read(SREG) & SREG_C);
    EXPECT_EQ(SREG_Z, sim->read(SREG) & SREG_Z);
    EXPECT_EQ(0, sim->read(SREG) & SREG_N);
    EXPECT_EQ(SREG_H, sim->read(SREG) & SREG_H);
    EXPECT_EQ(0, sim->read(16));
    EXPECT_EQ(1, sim->read(17));
}

TEST(add, sreg_signed_flags)
{
    // ldi r16,127      oooo KKKK dddd KKKK
    uint16_t ldi_rd = 0b1110'0111'0000'1111;

    // ldi r17,1        oooo KKKK dddd KKKK
    uint16_t ldi_rr = 0b1110'0000'0001'0001;

    // add r16,r17   opop'op'r'ddddd'rrrr
    uint16_t add = 0b0000'11'1'10000'0001;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi_rd);
    instr_to_bytes(text_bytes, ldi_rr);
    instr_to_bytes(text_bytes, add);
    instr_to_bytes(text_bytes, add);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(SREG_V, sim->read(SREG) & SREG_V);
    EXPECT_EQ(SREG_N, sim->read(SREG) & SREG_N);
    EXPECT_EQ(0, sim->read(SREG) & SREG_S);
    EXPECT_EQ(1<<7, sim->read(16));
    EXPECT_EQ(1, sim->read(17));

    sim->step();

    EXPECT_EQ(0, sim->read(SREG) & SREG_V);
    EXPECT_EQ(SREG_N, sim->read(SREG) & SREG_N);
    EXPECT_EQ(SREG_S, sim->read(SREG) & SREG_S);
    EXPECT_EQ(0x81, sim->read(16));
    EXPECT_EQ(1, sim->read(17));
}

TEST(adc, sum)
{
    // ldi r16,255      oooo KKKK dddd KKKK
    uint16_t ldi_rd = 0b1110'1111'0000'1111;

    // ldi r17,1        oooo KKKK dddd KKKK
    uint16_t ldi_rr = 0b1110'0000'0001'0001;

    // adc r16,r17   opop'op'r'ddddd'rrrr
    uint16_t adc = 0b0001'11'1'10000'0001;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi_rd);
    instr_to_bytes(text_bytes, ldi_rr);
    instr_to_bytes(text_bytes, adc);
    instr_to_bytes(text_bytes, adc);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(0, sim->read(16));
    EXPECT_EQ(1, sim->read(17));

    sim->step();

    EXPECT_EQ(2, sim->read(16));
    EXPECT_EQ(1, sim->read(17));
}

TEST(adc, sreg_unsigned_flags)
{
    // ldi r16,255      oooo KKKK dddd KKKK
    uint16_t ldi_rd = 0b1110'1111'0000'1111;

    // ldi r17,1        oooo KKKK dddd KKKK
    uint16_t ldi_rr = 0b1110'0000'0001'0001;

    // adc r16,r17   opop'op'r'ddddd'rrrr
    uint16_t adc = 0b0001'11'1'10000'0001;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi_rd);
    instr_to_bytes(text_bytes, ldi_rr);
    instr_to_bytes(text_bytes, adc);
    instr_to_bytes(text_bytes, adc);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(SREG_C, sim->read(SREG) & SREG_C);
    EXPECT_EQ(SREG_Z, sim->read(SREG) & SREG_Z);
    EXPECT_EQ(0, sim->read(SREG) & SREG_N);
    EXPECT_EQ(0, sim->read(16));
    EXPECT_EQ(1, sim->read(17));

    sim->step();

    EXPECT_EQ(0, sim->read(SREG) & SREG_C);
    EXPECT_EQ(0, sim->read(SREG) & SREG_Z);
    EXPECT_EQ(0, sim->read(SREG) & SREG_N);
    EXPECT_EQ(2, sim->read(16));
    EXPECT_EQ(1, sim->read(17));
}

TEST(adc, sreg_signed_flags)
{
    // ldi r16,127      oooo KKKK dddd KKKK
    uint16_t ldi_rd = 0b1110'0111'0000'1111;

    // ldi r17,1        oooo KKKK dddd KKKK
    uint16_t ldi_rr = 0b1110'0000'0001'0001;

    // adc r16,r17   opop'op'r'ddddd'rrrr
    uint16_t adc = 0b0001'11'1'10000'0001;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi_rd);
    instr_to_bytes(text_bytes, ldi_rr);
    instr_to_bytes(text_bytes, adc);
    instr_to_bytes(text_bytes, adc);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(SREG_V, sim->read(SREG) & SREG_V);
    EXPECT_EQ(SREG_N, sim->read(SREG) & SREG_N);
    EXPECT_EQ(0, sim->read(SREG) & SREG_S);
    EXPECT_EQ(0x80, sim->read(16));
    EXPECT_EQ(1, sim->read(17));

    sim->step();

    EXPECT_EQ(0, sim->read(SREG) & SREG_V);
    EXPECT_EQ(SREG_N, sim->read(SREG) & SREG_N);
    EXPECT_EQ(SREG_S, sim->read(SREG) & SREG_S);
    EXPECT_EQ(0x81, sim->read(16));
    EXPECT_EQ(1, sim->read(17));
}

TEST(call, call)
{
    // ldi r16,255   oooo kkkk dddd kkkk
    uint16_t ldi = 0b1110'1111'0000'1111;

    // sts r16,SPL   oooo ooo ddddd oooo
    uint32_t sts = 0b1001'001'10000'0000'0000'0000'0101'1101;

    // call 6         opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    uint32_t call = 0b1001010'00000'111'0'0000000000000110;

    uint16_t unreachable = 0b1001'0111'01'01'0110;

    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t add =      0b1001'0110'01'01'0110;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi);
    instr_to_bytes(text_bytes, sts);
    instr_to_bytes(text_bytes, call);
    instr_to_bytes(text_bytes, unreachable);
    instr_to_bytes(text_bytes, add);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();

    auto sp1 = stack_pointer(*sim);
    sim->step();
    auto sp2 = stack_pointer(*sim);

    EXPECT_EQ(decode_raw<16>(add), sim->next_instruction());
    EXPECT_EQ(sp1 - 2, sp2);
    EXPECT_EQ(5, sim->read(sp1));
    EXPECT_EQ(0, sim->read(sp1 + 1));
}

TEST(rcall, rcall)
{
    // ldi r16,255   oooo kkkk dddd kkkk
    uint16_t ldi = 0b1110'1111'0000'1111;

    // sts r16,SPL   oooo ooo ddddd oooo
    uint32_t sts = 0b1001'001'10000'0000'0000'0000'0101'1101;

    // rcall 1         oooo kkkk kkkk kkkk
    uint16_t rcall = 0b1101'0000'0000'0001;

    uint16_t unreachable = 0b1001'0111'01'01'0110;

    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t add =      0b1001'0110'01'01'0110;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi);
    instr_to_bytes(text_bytes, sts);
    instr_to_bytes(text_bytes, rcall);
    instr_to_bytes(text_bytes, unreachable);
    instr_to_bytes(text_bytes, add);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();

    auto sp1 = stack_pointer(*sim);
    sim->step();
    auto sp2 = stack_pointer(*sim);

    EXPECT_EQ(decode_raw<16>(add), sim->next_instruction());
    EXPECT_EQ(sp1 - 2, sp2);
    EXPECT_EQ(4, sim->read(sp1));
    EXPECT_EQ(0, sim->read(sp1 + 1));
}

TEST(ret, from_call)
{
    // ldi r16,255   oooo kkkk dddd kkkk
    uint16_t ldi = 0b1110'1111'0000'1111;

    // sts r16,SPL   oooo ooo ddddd oooo
    uint32_t sts = 0b1001'001'10000'0000'0000'0000'0101'1101;

    // call 6         opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    uint32_t call = 0b1001010'00000'111'0'0000000000000110;

    // sbiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t sub =      0b1001'0111'01'01'0110;

    // ret
    uint16_t ret = 0b1001'0101'0000'1000;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi);
    instr_to_bytes(text_bytes, sts);
    instr_to_bytes(text_bytes, call);
    instr_to_bytes(text_bytes, sub);
    instr_to_bytes(text_bytes, ret);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(decode_raw<16>(sub), sim->next_instruction());
}

TEST(ret, from_rcall)
{
    // ldi r16,255   oooo kkkk dddd kkkk
    uint16_t ldi = 0b1110'1111'0000'1111;

    // sts r16,SPL   oooo ooo ddddd oooo
    uint32_t sts = 0b1001'001'10000'0000'0000'0000'0101'1101;

    // rcall 1         oooo kkkk kkkk kkkk
    uint16_t rcall = 0b1101'0000'0000'0001;

    // sbiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t sub =      0b1001'0111'01'01'0110;

    // ret
    uint16_t ret = 0b1001'0101'0000'1000;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi);
    instr_to_bytes(text_bytes, sts);
    instr_to_bytes(text_bytes, rcall);
    instr_to_bytes(text_bytes, sub);
    instr_to_bytes(text_bytes, ret);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(decode_raw<16>(sub), sim->next_instruction());
}

TEST(jmp, jmp)
{
    // jmp 3         opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    uint32_t jmp = 0b1001010'00000'110'0'0000000000000011;

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

    // brge 1         oooo oo kkkkkkk ooo
    uint16_t brge = 0b1111'01'0000001'100;

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

    // brge 1         oooo oo kkkkkkk ooo
    uint16_t brge = 0b1111'01'0000001'100;

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

TEST(eor, eor)
{
    // ldi r16,1       oooo KKKK dddd KKKK
    uint16_t ldi16 = 0b1110'0000'0000'0001;

    // ldi r17,3       oooo KKKK dddd KKKK
    uint16_t ldi17 = 0b1110'0000'0001'0011;

    // eor r17,r16   oooo oo r ddddd rrrr
    uint16_t eor = 0b0010'01'1'10001'0000;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi16);
    instr_to_bytes(text_bytes, ldi17);
    instr_to_bytes(text_bytes, eor);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(1^3, sim->read(17));
}

TEST(lpm, low_byte)
{
    // ldi LO_Z,255  oooo KKKK dddd KKKK
    uint16_t ldi = 0b1110'1111'1110'1111;

    // lpm r2        oooo ooo ddddd oooo
    uint16_t lpm = 0b1001'000'00010'0101;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi);
    instr_to_bytes(text_bytes, lpm);

    auto text = text_segment(text_bytes);
    auto data = std::make_unique<mock_segment>(2, 255, std::vector<byte_t>{1,2});
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>{data.get()});

    sim->step();
    sim->step();

    EXPECT_EQ(1, sim->read(R2));
    EXPECT_EQ(0, sim->read(Z_LO));
    EXPECT_EQ(1, sim->read(Z_HI));
}

TEST(lpm, high_byte)
{
    // ldi LO_Z,255     oooo KKKK dddd KKKK
    uint16_t ldi_lo = 0b1110'1111'1110'1111;

    // ldi HI_Z,0x80    oooo KKKK dddd KKKK
    uint16_t ldi_hi = 0b1110'1000'1111'0000;

    // lpm r2        oooo ooo ddddd oooo
    uint16_t lpm = 0b1001'000'00010'0101;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi_lo);
    instr_to_bytes(text_bytes, ldi_hi);
    instr_to_bytes(text_bytes, lpm);

    auto text = text_segment(text_bytes);
    auto data = std::make_unique<mock_segment>(2, 255, std::vector<byte_t>{1,2});
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>{data.get()});

    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(2, sim->read(R2));
    EXPECT_EQ(0, sim->read(Z_LO));
    EXPECT_EQ(0x81, sim->read(Z_HI));
}

TEST(push, push)
{
    // ldi r16,255   oooo kkkk dddd kkkk
    uint16_t ldi = 0b1110'1111'0000'1111;

    // sts r16,SPL   oooo ooo ddddd oooo
    uint32_t sts = 0b1001'001'10000'0000'0000'0000'0101'1101;

    // push r16       oooo ooo ddddd oooo
    uint16_t push = 0b1001'001'10000'1111;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi);
    instr_to_bytes(text_bytes, sts);
    instr_to_bytes(text_bytes, push);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(254, sim->read(SPL));
    EXPECT_EQ(0, sim->read(SPH));
    EXPECT_EQ(255, sim->read(255));
}

TEST(pop, pop)
{
    // ldi r16,254      oooo kkkk dddd kkkk
    uint16_t ldi_sp = 0b1110'1111'0000'1110;

    // ldi r17,1          oooo kkkk dddd kkkk
    uint16_t ldi_data = 0b1110'0000'0001'0001;

    // sts r16,SPL      oooo ooo ddddd oooo
    uint32_t sts_sp = 0b1001'001'10000'0000'0000'0000'0101'1101;

    // sts r17,255        oooo ooo ddddd oooo
    uint32_t sts_data = 0b1001'001'10001'0000'0000'0000'1111'1111;

    // pop r2        oooo ooo ddddd oooo
    uint16_t pop = 0b1001'000'00010'1111;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi_sp);
    instr_to_bytes(text_bytes, ldi_data);
    instr_to_bytes(text_bytes, sts_sp);
    instr_to_bytes(text_bytes, sts_data);
    instr_to_bytes(text_bytes, pop);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(255, sim->read(SPL));
    EXPECT_EQ(0, sim->read(SPH));
    EXPECT_EQ(1, sim->read(R2));
}

TEST(pop, after_push)
{
    // ldi r16,255   oooo kkkk dddd kkkk
    uint16_t ldi = 0b1110'1111'0000'1111;

    // sts r16,SPL   oooo ooo ddddd oooo
    uint32_t sts = 0b1001'001'10000'0000'0000'0000'0101'1101;

    // push r16       oooo ooo ddddd oooo
    uint16_t push = 0b1001'001'10000'1111;

    // pop r2        oooo ooo ddddd oooo
    uint16_t pop = 0b1001'000'00010'1111;

    std::vector<byte_t> text_bytes;
    instr_to_bytes(text_bytes, ldi);
    instr_to_bytes(text_bytes, sts);
    instr_to_bytes(text_bytes, push);
    instr_to_bytes(text_bytes, pop);

    auto text = text_segment(text_bytes);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment *>());

    sim->step();
    sim->step();
    sim->step();
    sim->step();

    EXPECT_EQ(255, sim->read(SPL));
    EXPECT_EQ(0, sim->read(SPH));
    EXPECT_EQ(255, sim->read(R2));
}
