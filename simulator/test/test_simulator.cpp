#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "avr/instruction.h"
#include "avr/register.h"
#include "segment.h"
#include "simulator.h"

using namespace avr;
using namespace simulator;

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

std::unique_ptr<segment> empty_segment()
{
    return std::make_unique<mock_segment>(0, 0, std::vector<byte_t>());
}

std::unique_ptr<segment> instr_segment(uint16_t instr, address_t addr = 0)
{
    std::vector<byte_t> seg;
    byte_t *bytes = reinterpret_cast<byte_t *>(&instr);
    seg.push_back(bytes[1]);
    seg.push_back(bytes[0]);
    return std::make_unique<mock_segment>(2, addr, std::move(seg));
}

TEST(adiw, sum)
{
    // adiw X,010110(22)  opop'opop'kk'pp'kkkk
    uint16_t instr =    0b1001'0110'01'01'0110;

    auto text = instr_segment(instr);
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

    auto text = instr_segment(instr);
    auto sim = program_with_segments(atmega168, *text, std::vector<segment>());
    sim->step();

    int16_t x;
    byte_t *bytes = reinterpret_cast<byte_t *>(&x);
    bytes[0] = sim->read(X_LO);
    bytes[1] = sim->read(X_HI);

    EXPECT_EQ(-22, x);
}
