#pragma once

#include "types.h"

namespace avr {

    enum reg
        : address_t
    {
        SREG = 0x5F,
        R2 = 0x02,
        R26 = 0x1A,
        R27 = 0x1B,
        R28 = 0x1C,
        R29 = 0x1D,
        R30 = 0x1E,
        R31 = 0x1F,
        SPL = 0x5D,
        SPH = 0x5E
    };

    static constexpr reg X_LO = R26;
    static constexpr reg X_HI = R27;
    static constexpr reg Y_LO = R28;
    static constexpr reg Y_HI = R29;
    static constexpr reg Z_LO = R30;
    static constexpr reg Z_HI = R31;


    enum sreg_flag
        : byte_t
    {
        SREG_H = 0b0010'0000,
        SREG_S = 0b0001'0000,
        SREG_V = 0b0000'1000,
        SREG_N = 0b0000'0100,
        SREG_Z = 0b0000'0010,
        SREG_C = 0b0000'0001
    };
}
