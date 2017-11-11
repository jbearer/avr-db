#pragma once

#include <memory>

#include "types.h"

namespace simulator {

    enum section_type_t {
        DATA,
        TEXT,
        BSS
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
