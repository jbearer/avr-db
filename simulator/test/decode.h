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
        return avr::decode(&raw);
    }

    template<>
    inline avr::instruction decode_raw<32>(uint32_t raw)
    {
        uint16_t words[2];
        auto raw_words = reinterpret_cast<uint16_t *>(&raw);
        words[0] = raw_words[1];
        words[1] = raw_words[0];
        return avr::decode(words);
    }
}
