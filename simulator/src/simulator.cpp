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

        reinterpret_cast<uint16_t &>(memory[SPL]) = board.ram_end - 2;
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

    instruction next_instruction() const override
    {
        return decode(&text[pc]);
    }

    void step() override
    {
        run_until([]() { return true; });
    }

    void next() override
    {
        auto cur_pc = pc;
        auto instr = next_instruction();
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
        run_until(stop, next_instruction());
    }

    void run_until(const std::function<bool()> & stop, const instruction & first_instr)
    {
        execute(first_instr);
        while (!stop()) {
            execute(next_instruction());
        }
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
            pc += instr.size;
            break;
        case SBIW:
            sbiw(instr.args.constant_register_pair.pair, instr.args.constant_register_pair.constant);
            pc += instr.size;
            break;
        case CALL:
            call(instr.args.address.address);
            break;
        case RET:
            ret();
            break;
        case JMP:
            jmp(instr.args.address.address);
            break;
        case STS:
            sts(instr.args.reg_address.reg, instr.args.reg_address.address);
            pc += instr.size;
            break;
        default:
            throw unimplemented_error(instr);
        }
    }

    void adiw(register_pair pair, uint16_t value)
    {
        uint16_t & reg = reinterpret_cast<uint16_t &>(memory[register_pair_address(pair)]);
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

        reg = static_cast<uint16_t>(result);
    }

    void sbiw(register_pair pair, uint16_t value)
    {
        adiw(pair, ~value + 1);
    }

    void call(address_t addr)
    {
        uint16_t & sp = reinterpret_cast<uint16_t &>(memory[SPL]);
        uint16_t & stack = reinterpret_cast<uint16_t &>(memory[sp]);
        stack = pc + 4; // Instruction after the call
        sp -= 2;
        pc = addr;
    }

    void ret()
    {
        uint16_t & sp = reinterpret_cast<uint16_t &>(memory[SPL]);
        sp += 2;
        pc = reinterpret_cast<uint16_t &>(memory[sp]);
    }

    void jmp(address_t addr)
    {
        pc = addr;
    }

    void sts(uint8_t reg, address_t address)
    {
        memory[address] = memory[reg];
    }

    std::vector<byte_t>         text;
    std::vector<bool>           breakpoints;
    std::vector<byte_t>         memory;
    uint16_t                    pc = 0;
    byte_t &                    sreg;
};

std::unique_ptr<simulator::simulator> simulator::program_with_segments(
    const avr::board & board, const segment & text, const std::vector<segment> & other_segs)
{
    return std::make_unique<simulator_impl>(board, text, other_segs);
}


