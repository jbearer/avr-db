#pragma once

namespace avr {

    static constexpr uint16_t kilobyte = 1024;

    struct board
    {
        const size_t ram_end;
        const size_t flash_end;

        board(size_t ram_end_, size_t flash_end_)
            : ram_end(ram_end_)
            , flash_end(flash_end_)
        {}
    };

    static const board atmega168(
        1*kilobyte,         // ram_end TODO is this right?
        (16*kilobyte)/2     // flash_end
    );
}
