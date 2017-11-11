#pragma once

#include "types.h"

namespace avr {

    enum reg
        : address_t
    {
        SREG = 0x5F,
        R26 = 0x1A,
        R27 = 0x1B
    };

    static constexpr reg X_LO = R26;
    static constexpr reg X_HI = R27;

    enum sreg_flag
        : byte_t
    {
        SREG_S = 0b0001'0000,
        SREG_V = 0b0000'1000,
        SREG_N = 0b0000'0100,
        SREG_Z = 0b0000'0010,
        SREG_C = 0b0000'0001
    };
}
