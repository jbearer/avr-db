#include "segment.h"
#include "elfio/elfio.hpp"
#include <array>

using namespace simulator;

struct text_segment : segment {

    size_t size() const
    {
        return 0;
    }


private:

    static const size_t memory_size = 1024;

    size_t size_;
    // offset address for the beginning of the segment in virtual memory
    address_t address_;
    std::array<byte_t, memory_size> memory;     // holds the physical memory

};



