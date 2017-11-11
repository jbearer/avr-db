#pragma once

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "avr/boards.h"
#include "avr/instruction.h"
#include "segment.h"

namespace simulator {

    struct unimplemented_error
        : std::exception
    {
        unimplemented_error(const avr::instruction & instr);

        const char *what() const noexcept override;

    private:
        std::string desc;
    };

    struct simulator
    {
        virtual void set_breakpoint(address_t) = 0;
        virtual void delete_breakpoint(address_t) = 0;
        virtual byte_t read(address_t) const = 0;
        virtual avr::instruction next_instruction() const = 0;
        virtual void step() = 0;
        virtual void next() = 0;
        virtual void run() = 0;
    };

    std::unique_ptr<simulator> program_with_segments(
        const avr::board & board, const segment & text, const std::vector<segment *> & other_segs);

}
