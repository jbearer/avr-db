#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include <iostream>

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
    simulator_impl(const avr::board & board, const segment & text_seg, const std::vector<segment *> & other_segs)
        : text(board.flash_end)
        , breakpoints(board.flash_end, false)
        , memory(board.ram_end)
        , sreg(memory[reg::SREG])
    {
        auto text_it = text.begin();
        std::advance(text_it, text_seg.address());
        auto text_words = text_seg.data<uint16_t>();
        std::copy(text_words, text_words + text_seg.count<uint16_t>(), text_it);

        for (auto other_seg : other_segs) {
            auto flash_it = text.begin();
            std::advance(flash_it, other_seg->address());
            auto data_words = other_seg->data<uint16_t>();
            std::copy(data_words, data_words + other_seg->count<uint16_t>(), flash_it);
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
            call(instr.args.address.address, pc + instr.size);
            break;
        case RCALL:
            rcall(instr.args.offset12.offset, pc + instr.size);
            pc += instr.size;
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
        case CP:
            cp(instr.args.register1_register2.register1, instr.args.register1_register2.register2);
            pc += instr.size;
            break;
        case CPC:
            cpc(instr.args.register1_register2.register1, instr.args.register1_register2.register2);
            pc += instr.size;
            break;
        case ADD:
            add(instr.args.register1_register2.register1, instr.args.register1_register2.register2);
            pc += instr.size;
            break;
        case ADC:
            adc(instr.args.register1_register2.register1, instr.args.register1_register2.register2);
            pc += instr.size;
            break;
        case LDI:
            ldi(instr.args.constant_register.reg, instr.args.constant_register.constant);
            pc += instr.size;
            break;
        case CPI:
            cpi(instr.args.constant_register.reg, instr.args.constant_register.constant);
            pc += instr.size;
            break;
        case LDS:
            lds(instr.args.reg_address.reg, instr.args.reg_address.address);
            pc += instr.size;
            break;
        case BRGE:
            brge(instr.args.offset.offset);
            pc += instr.size;
            break;
        case BRNE:
            brne(instr.args.offset.offset);
            pc += instr.size;
            break;
        case RJMP:
            rjmp(instr.args.offset12.offset);
            pc += instr.size;
            break;
        case EOR:
            eor(instr.args.register1_register2.register1, instr.args.register1_register2.register2);
            pc += instr.size;
            break;
        case IN:
            in(instr.args.ioaddress_register.ioaddress, instr.args.ioaddress_register.reg);
            pc += instr.size;
            break;
        case OUT:
            out(instr.args.ioaddress_register.ioaddress, instr.args.ioaddress_register.reg);
            pc += instr.size;
            break;
        case LPM:
            lpm(instr.args.reg.reg);
            pc += instr.size;
            break;
        case STX:
            stx(instr.args.reg.reg);
            pc += instr.size;
            break;
        case PUSH:
            push(memory[instr.args.reg.reg]);
            pc += instr.size;
            break;
        case POP:
            memory[instr.args.reg.reg] = pop();
            pc += instr.size;
            break;
        default:
            throw unimplemented_error(instr);
        }
    }

    void add_to_reg(uint8_t & reg, uint8_t del)
    {
        uint16_t result = reg + del;

        // Check for signed overflow
        int8_t signed_reg = static_cast<int16_t>(reg);
        int8_t signed_del = static_cast<int16_t>(del);
        int8_t signed_result = result & 0xFF;
        toggle_sreg_flag(SREG_V,
            (signed_reg > 0 && signed_del > 0 && signed_result <= 0) ||
            (signed_reg < 0 && signed_del < 0 && signed_result >= 0));

        // Half-carry flag
        toggle_sreg_flag(SREG_H,
            (result & (1<<4)) ? !(!!(reg & (1<<4)) ^ !!(del & (1<<4)))
                              :  (!!(reg & (1<<4)) ^ !!(del & (1<<4))));

        // Check MSB of result
        toggle_sreg_flag(SREG_N, result & (1<<7));

        // Check for zero result
        toggle_sreg_flag(SREG_Z, !(result & 0xFF));

        // Check for carry
        toggle_sreg_flag(SREG_C, result & (1<<8));

        update_sreg_sign();

        reg = result;
    }

    void sub_from_reg(uint8_t & reg, uint8_t del)
    {
        uint8_t old_reg = reg;
        add_to_reg(reg, ~del + 1);
        toggle_sreg_flag(SREG_C, del > old_reg);
    }

    void adiw(register_pair pair, uint16_t value)
    {
        // Save half-carry flag: adiw should not modify it
        uint8_t h = sreg | ~SREG_H;

        auto lo_reg = register_pair_address(pair);
        auto hi_reg = lo_reg + 1;
        add_to_reg(memory[lo_reg], value & 0xFF);
        add_to_reg(memory[hi_reg], ((value & 0xFF00) >> 8) + !!(sreg & SREG_C));

        // Restore half-carry flag
        sreg &= h;
    }

    void sbiw(register_pair pair, uint16_t value)
    {
        // Save half-carry flag: sbiw should not modify it
        uint8_t h = sreg | ~SREG_H;

        auto lo_reg = register_pair_address(pair);
        auto hi_reg = lo_reg + 1;
        sub_from_reg(memory[lo_reg], value & 0xFF);
        sub_from_reg(memory[hi_reg], ((value & 0xFF00) >> 8) + !!(sreg & SREG_C));

        // Restore half-carry flag
        sreg &= h;
    }

    void add(uint8_t r1, uint8_t r2)
    {
        auto & rr = memory[r1];
        auto & rd = memory[r2];
        add_to_reg(rd, rr);
    }

    void adc(uint8_t r1, uint8_t r2)
    {
        auto & rr = memory[r1];
        auto & rd = memory[r2];
        add_to_reg(rd, rr + !!(sreg & SREG_C));
    }

    void push(uint8_t b)
    {
        uint16_t & sp = reinterpret_cast<uint16_t &>(memory[SPL]);
        memory[sp--] = b;
    }

    uint8_t pop()
    {
        uint16_t & sp = reinterpret_cast<uint16_t &>(memory[SPL]);
        return memory[++sp];
    }

    void call(uint16_t jump_to, uint16_t return_to)
    {
        push(return_to & 0x00FF);
        push(return_to & 0xFF00);
        pc = jump_to;
    }

    void rcall(int16_t offset, uint16_t return_to)
    {
        call(pc + offset, return_to);
    }

    void ret()
    {
        uint16_t addr = 0;
        addr |= pop() << 8;
        addr |= pop();
        pc = addr;
    }

    void jmp(address_t addr)
    {
        pc = addr;
    }

    void sts(uint8_t reg, address_t address)
    {
        memory[address] = memory[reg];
    }

    void cp(uint8_t r1, uint8_t r2)
    {
        auto rr = memory[r1];
        auto rd = memory[r2];

        int16_t res = (int16_t)rd - (int16_t)rr;
        // TODO implement half-carry flag

        toggle_sreg_flag(SREG_V,
            res < std::numeric_limits<int8_t>::min() ||
            res > std::numeric_limits<int8_t>::max());

        toggle_sreg_flag(SREG_Z, !(res & 0xFF));
        toggle_sreg_flag(SREG_C, rr > rd);
        toggle_sreg_flag(SREG_N, res & (1<<7));
        update_sreg_sign();
    }

    void cpc(uint8_t r1, uint8_t r2)
    {
        auto rr = memory[r1];
        auto rd = memory[r2];
        auto carry = !!(sreg & SREG_C);

        int16_t res = (int16_t)rd - (int16_t)rr - (int16_t)carry;
        // TODO implement half-carry flag

        toggle_sreg_flag(SREG_V,
            res < std::numeric_limits<int8_t>::min() ||
            res > std::numeric_limits<int8_t>::max());

        if (res & 0xFF) {
            sreg &= ~SREG_Z;
        }

        toggle_sreg_flag(SREG_C, rr + carry > rd);
        toggle_sreg_flag(SREG_N, res & (1<<7));
        update_sreg_sign();
    }

    void eor(uint8_t r1, uint8_t r2)
    {
        auto & rr = memory[r1];
        auto & rd = memory[r2];
        rd ^= rr;

        sreg &= ~SREG_V;
        toggle_sreg_flag(SREG_N, rd & (1 << 7));
        toggle_sreg_flag(SREG_Z, !rd);
        update_sreg_sign();
    }

    void ldi(uint8_t reg, uint8_t val)
    {
        memory[reg] = val;
    }

    void cpi(uint8_t reg, uint8_t val)
    {
        int16_t res = (int16_t)memory[reg] - (int16_t)val;

        // TODO implement half-carry

        toggle_sreg_flag(SREG_V,
            res < std::numeric_limits<int8_t>::min() ||
            res > std::numeric_limits<int8_t>::max());
        toggle_sreg_flag(SREG_Z, !(res & 0xFF));
        toggle_sreg_flag(SREG_C, val > memory[reg]);
        update_sreg_sign();
    }

    void lds(uint8_t reg, address_t address)
    {
        memory[reg] = memory[address];
    }

    void brge(int8_t offset)
    {
        if (!(sreg & SREG_S)) {
            pc += offset;
        }
    }

    void brne(int8_t offset)
    {
        if (!(sreg & SREG_Z)) {
            pc += offset;
        }
    }

    void rjmp(int16_t offset)
    {
        pc += offset;
    }

    void in(int8_t ioaddress, int8_t reg)
    {
        memory[reg] = memory[ioaddress + 0x20];
    }

    void out(int8_t ioaddress, int8_t reg)
    {
        memory[ioaddress + 0x20] = memory[reg];
    }

    void lpm(uint8_t reg)
    {
        auto & z = reinterpret_cast<uint16_t &>(memory[Z_LO]);
        uint16_t word = text[z & 0x7FFF];
        memory[reg] = (z & (1 << 15)) ? (word & 0xFF00) >> 8 : word & 0xFF;
        ++z;
    }

    void stx(uint8_t reg)
    {
        auto & x = reinterpret_cast<uint16_t &>(memory[X_LO]);
        memory[x] = memory[reg];
        ++x;
    }

    std::vector<uint16_t>   text;
    std::vector<bool>       breakpoints;
    std::vector<uint8_t>    memory;
    uint16_t                pc = 0;
    byte_t &                sreg;
};

std::unique_ptr<simulator::simulator> simulator::program_with_segments(
    const avr::board & board, const segment & text, const std::vector<segment *> & other_segs)
{
    return std::make_unique<simulator_impl>(board, text, other_segs);
}
