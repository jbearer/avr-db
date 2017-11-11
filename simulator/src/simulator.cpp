#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "avr/boards.h"
#include "avr/instruction.h"
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
    simulator_impl(const avr::board & board, const segment & text_seg, const segment & data_seg)
        : text(board.flash_end)
        , breakpoints(board.flash_end, false)
        , memory(board.ram_end)
    {
        auto text_it = text.begin();
        std::advance(text_it, text_seg.address());
        std::copy(text_seg.data(), text_seg.data() + text_seg.size(), text_it);

        auto mem_it = memory.begin();
        std::advance(mem_it, data_seg.address());
        std::copy(data_seg.data(), data_seg.data() + data_seg.size(), mem_it);
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
        auto instr = decode(memory.data() + cur_pc);
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
        run_until(stop, decode(memory.data() + pc));
    }

    void run_until(const std::function<bool()> & stop, const instruction & first_instr)
    {
        execute(first_instr);
        while (!stop()) {
            execute(decode(memory.data() + pc));
        }
    }

    void execute(const instruction & instr)
    {
        switch (instr.op) {
        default:
            throw unimplemented_error(instr);
        }

        pc += instr.size;
    }

    std::vector<byte_t>         text;
    std::vector<bool>           breakpoints;
    std::vector<byte_t>         memory;
    address_t                   pc;
};
