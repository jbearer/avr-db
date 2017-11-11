#include <iostream>
#include <string>

#include "avr/boards.h"
#include "segment.h"
#include "simulator.h"

using namespace simulator;

void repl(simulator::simulator & sim)
{
    std::cout << avr::mnemonic(sim.next_instruction()) << '\n';

    char command;
    while (std::cin >> command) {
        switch (command) {
        case 's':
            sim.step();
            break;
        }
        std::cout << avr::mnemonic(sim.next_instruction()) << '\n';
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <elf>\n";
        return 1;
    }

    std::string elf = argv[1];
    auto text = map_segment(elf, TEXT);
    auto data = map_segment(elf, DATA);
    auto bss = map_segment(elf, BSS);

    std::vector<segment *> ram_segs;
    if (data->size() > 0) {
        ram_segs.push_back(data.get());
    }
    if (bss->size() > 0) {
        ram_segs.push_back(bss.get());
    }

    auto sim = program_with_segments(avr::atmega168, *text, ram_segs);
    repl(*sim);
}
