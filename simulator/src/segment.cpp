#include "segment.h"
#include "elfio/elfio.hpp"
#include <array>

using namespace simulator;

struct text_segment : segment {

    text_segment(std::string path, simulator::section_type_t section_type)
    {
        ELFIO::elfio reader;
        if (!reader.load(path)) {
            std::cout << "loading file failed" << std::endl;
        } else {
            std::cout << "successfully loaded " << path << std::endl;
        }

        // TODO: THIS IS BAD!!! relies on the segments being ordered the same
        // way each time.  Eventually we should fis this.
        ELFIO::segment* elf_segment;
        switch (section_type) {
            case TEXT:
                elf_segment = reader.segments[0];
                break;
            case DATA:
                elf_segment = reader.segments[1];
                break;
            case BSS:
                elf_segment = reader.segments[2];
                break;
            default:
                std::cout << "ERROR: no name for this type" << std::endl;
        }

        const char* segment_start = elf_segment->get_data();

        for (size_t i = 0; i < elf_segment->get_file_size(); ++i) {
            memory_.push_back(*(segment_start + i));
        }

        size_ = elf_segment->get_file_size();
        address_ = elf_segment->get_physical_address();
    }

    virtual size_t size() const
    {
        return size_;
    }

    virtual address_t address() const
    {
        return address_;
    }

    const byte_t *data() const
    {
        return memory_.data();
    }


private:

    size_t size_;
    // offset address for the beginning of the segment in virtual memory
    address_t address_;
    std::vector<byte_t> memory_;     // holds the physical memory vector
};


std::unique_ptr<simulator::segment> simulator::map_segment(
    std::string fname, simulator::section_type_t section)
{
    return std::make_unique<text_segment>(fname, section);
}



