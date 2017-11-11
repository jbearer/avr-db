#pragma once

#include <memory>

namespace simulator {

    using address_t = size_t;
    using byte_t = uint8_t;

    enum section_type_t {
        DATA,
        TEXT
    };

    struct segment
    {
        virtual size_t size() const = 0;
        virtual address_t address() const = 0;
        virtual const byte_t *data() const = 0;
        virtual ~segment() {}
    };

    std::unique_ptr<segment> map_segment(
        std::string fname, section_type_t section);

}
