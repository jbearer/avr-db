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

void instrs_to_bytes(std::vector<byte_t> &) {}

template<class... Args>
void instrs_to_bytes(std::vector<byte_t> & v, uint16_t instr, Args... args)
{
    byte_t *bytes = reinterpret_cast<byte_t *>(&instr);
    v.push_back(bytes[1]);
    v.push_back(bytes[0]);
    instrs_to_bytes(v, args...);
}

template<class... Args>
void instrs_to_bytes(std::vector<byte_t> & v, uint32_t instr, Args... args)
{
    byte_t *bytes = reinterpret_cast<byte_t *>(&instr);
    v.push_back(bytes[3]);
    v.push_back(bytes[2]);
    v.push_back(bytes[1]);
    v.push_back(bytes[0]);
    instrs_to_bytes(v, args...);
}

std::unique_ptr<segment> empty_segment()
{
    return std::make_unique<mock_segment>(0, 0, std::vector<byte_t>());
}

template<class... Instrs>
std::unique_ptr<segment> text_segment(Instrs... instrs)
{
    std::vector<byte_t> seg;
    instrs_to_bytes(seg, instrs...);
    auto size = seg.size();
    return std::make_unique<mock_segment>(size, 0, std::move(seg));
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

    auto text = text_segment(instr);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment>());
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

    auto text = text_segment(instr);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment>());
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

    auto text = text_segment(call, unreachable, add);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment>());

    auto sp1 = stack_pointer(*sim);
    sim->step();
    auto sp2 = stack_pointer(*sim);

    EXPECT_EQ(decode_raw<16>(add), sim->next_instruction());
    EXPECT_EQ(sp1 - 2, sp2);
    EXPECT_EQ(0, sim->read(sp1));
    EXPECT_EQ(0, sim->read(sp1 + 1));
}

TEST(jmp, jmp)
{
    // jmp 6         opopopo'kkkkk'opo'k'kkkkkkkkkkkkkkkk
    uint32_t jmp = 0b1001010'00000'110'0'0000000000000110;

    uint16_t unreachable = 0b1001'0111'01'01'0110;

    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t add =      0b1001'0110'01'01'0110;

    auto text = text_segment(jmp, unreachable, add);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment>());

    sim->step();
    EXPECT_EQ(decode_raw<16>(add), sim->next_instruction());
}
