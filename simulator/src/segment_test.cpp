#include "segment.h"
#include <iostream>

using namespace simulator;

int main()
{
    std::cout << "hi there, you're doing great :)" << std::endl;
    std::string path = "simple.elf";
    map_segment(path, section_type_t::TEXT);

    return 0;
}
