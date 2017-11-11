#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "avr/boards.h"
#include "avr/instruction.h"
#include "avr/register.h"
#include "segment.h"
#include "simulator.h"

using namespace std::string_literals;

using namespace avr;
using namespace simulator;

unimplemented_error::unimplemented_error(const instruction & instr)
    : desc("unimplemented instruction: "s + mnemonic(instr))
{}

const char *unimplemented_error::what() const noexcept
{
    return desc.c_str();
}

struct simulator_impl
    : simulator::simulator
{
    simulator_impl(const avr::board & board, const segment & text_seg, const std::vector<segment> & other_segs)
        : text(board.flash_end)
        , breakpoints(board.flash_end, false)
        , memory(board.ram_end)
        , sreg(memory[reg::SREG])
    {
        auto text_it = text.begin();
        std::advance(text_it, text_seg.address());
        std::copy(text_seg.data(), text_seg.data() + text_seg.size(), text_it);

        for (auto & other_seg : other_segs) {
            auto mem_it = memory.begin();
            std::advance(mem_it, other_seg.address());
            std::copy(other_seg.data(), other_seg.data() + other_seg.size(), mem_it);
        }
    }

    void set_breakpoint(address_t address) override
    {
        breakpoints[address] = true;
    }

    void delete_breakpoint(address_t address) override
    {
        breakpoints[address] = false;
    }

    byte_t read(address_t address) const override
    {
        return memory[address];
    }

    void step() override
    {
        run_until([]() { return true; });
    }

    void next() override
    {
        auto cur_pc = pc;
        auto instr = decode(text.data() + cur_pc);
        switch (instr.op) {
        case CALL:
            run_until([this, cur_pc, &instr]() { return pc == cur_pc + instr.size; }, instr);
            break;
        default:
            run_until([]() { return true; }, instr);
        }
    }

    void run() override
    {
        run_until([this]() { return breakpoints[pc]; });
    }

private:

    void run_until(const std::function<bool()> & stop)
    {
        run_until(stop, decode(text.data() + pc));
    }

    void run_until(const std::function<bool()> & stop, const instruction & first_instr)
    {
        execute(first_instr);
        while (!stop()) {
            execute(decode(text.data() + pc));
        }
    }

    uint16_t read_register_pair(register_pair pair)
    {
        uint16_t reg;
        byte_t *reg_bytes = reinterpret_cast<byte_t *>(&reg);
        auto reg_addr = register_pair_address(pair);
        reg_bytes[0] = memory[reg_addr + 1];
        reg_bytes[1] = memory[reg_addr];
        return reg;
    }

    void write_register_pair(register_pair pair, uint16_t value)
    {
        auto reg_addr = register_pair_address(pair);
        reinterpret_cast<uint16_t &>(memory[reg_addr]) = value;
    }

    void toggle_sreg_flag(sreg_flag bit, bool test)
    {
        if (test) {
            sreg |= bit;
        } else {
            sreg &= ~bit;
        }
    }

    void update_sreg_sign()
    {
        // SREG should obey the invariant that S = N XOR V
        toggle_sreg_flag(SREG_S, !(sreg & SREG_N) != !(sreg & SREG_V));
    }

    void execute(const instruction & instr)
    {
        switch (instr.op) {
        case ADIW:
            adiw(instr.args.constant_register_pair.pair, instr.args.constant_register_pair.constant);
            break;
        default:
            throw unimplemented_error(instr);
        }

        pc += instr.size;
    }

    void adiw(register_pair pair, uint16_t value)
    {
        uint16_t reg = read_register_pair(pair);
        uint32_t result = reg + value;
        int32_t sresult = static_cast<int32_t>(result);

        // Check for signed overflow
        toggle_sreg_flag(SREG_V,
            sresult > std::numeric_limits<int16_t>::max() ||
            sresult < std::numeric_limits<int16_t>::min());

        // Check MSB of result
        toggle_sreg_flag(SREG_N, result & (1<<15));

        // Check for zero result
        toggle_sreg_flag(SREG_Z, !result);

        // Check for carry
        toggle_sreg_flag(SREG_C, result & (1<<16));

        update_sreg_sign();

        write_register_pair(pair, static_cast<uint16_t>(result));
    }

    std::vector<byte_t>         text;
    std::vector<bool>           breakpoints;
    std::vector<byte_t>         memory;
    address_t                   pc = 0;
    byte_t &                    sreg;
};

std::unique_ptr<simulator::simulator> simulator::program_with_segments(
    const avr::board & board, const segment & text, const std::vector<segment> & other_segs)
{
    return std::make_unique<simulator_impl>(board, text, other_segs);
}


