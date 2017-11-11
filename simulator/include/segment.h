#pragma once

#include <memory>

namespace simulator {

    using address_t = size_t;
    using byte_t = uint8_t;

    struct segment
    {
        virtual size_t size() const;
        virtual address_t address() const;
        virtual byte_t *data();
        virtual const byte_t *data() const;
        virtual ~segment() {}
    };

    std::unique_ptr<segment> map_segment(
        int fd, size_t offset, size_t size, address_t address);

}
