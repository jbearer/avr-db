#include "segment.h"
#include <iostream>

using namespace simulator;

int main()
{
    std::cout << "hi there, you're doing great :)" << std::endl;
    std::string path = "simple.elf";
    auto segment = map_segment(path, section_type_t::DATA);

    std::cout << std::hex << (long) segment->address() << std::endl;
    std::cout << std::hex << (long) segment->size() << std::endl;

    return 0;
}
