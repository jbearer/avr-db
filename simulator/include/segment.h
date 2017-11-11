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
        virtual const byte_t *bytes() const = 0;
        virtual ~segment() {}

        template<class T>
        const T *data() const
        {
            return reinterpret_cast<const T *>(bytes());
        }

        template<class T>
        size_t count() const
        {
            return size() / sizeof(T);
        }
    };

    std::unique_ptr<segment> map_segment(
        std::string fname, section_type_t section);
}
