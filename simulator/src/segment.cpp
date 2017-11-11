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

        ELFIO::section* section;
        if (section_type == section_type_t::DATA) {
            section = reader.sections[".data"];
        }
        else if (section_type == section_type_t::TEXT) {
            section = reader.sections[".text"];
        }
        else {
            std::cout << "ERRROR: unsupported section type" << std::endl;
        }

        const char* section_start = section->get_data();

        for (size_t i = 0; i < section->get_size(); ++i) {
            memory_.push_back(*(section_start + i));
        }

        size_ = section->get_size();
        address_ = section->get_address();

        for (auto& data : memory_) {
            std::cout << std::hex << (long) data;
        }

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
    std::string fname, simulator::section_type_t section/*, size_t offset, size_t size, address_t address*/)
{
    return std::make_unique<text_segment>(fname, section);
}



