#pragma once

#include "avr/instruction.h"

namespace testing {
    template<size_t>
    struct uint_n;

    template<>
    struct uint_n<16>
    {
        using type = uint16_t;
    };

    template<>
    struct uint_n<32>
    {
        using type = uint32_t;
    };

    template<size_t n> using uint_n_t = typename uint_n<n>::type;

    template<size_t n> avr::instruction decode_raw(uint_n_t<n>);

    template<>
    inline avr::instruction decode_raw<16>(uint16_t raw)
    {
        byte_t *bytes = reinterpret_cast<byte_t *>(&raw);
        byte_t pc[2];

        // Convert from little-endian
        pc[0] = bytes[1];
        pc[1] = bytes[0];

        return avr::decode(pc);
    }

    template<>
    inline avr::instruction decode_raw<32>(uint32_t raw)
    {
        byte_t *bytes = reinterpret_cast<byte_t *>(&raw);
        byte_t pc[4];

        // Convert from little-endian
        pc[0] = bytes[3];
        pc[1] = bytes[2];
        pc[2] = bytes[1];
        pc[3] = bytes[0];

        return avr::decode(pc);
    }
}