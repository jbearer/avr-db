#include "rduima.h"
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

char rduima::read(int address) {
    ofstream writer;
    writer.open(this->port);

    if (writer.is_open()) {
        writer << 0;
        writer << '\n';
        writer << address;
        writer << '\n';
        writer.close();
    }

    ifstream reader;
    reader.open(this->port);
    char * buffer = new char[100];

    if (reader.is_open()) {
        while (buffer == 0) {
            reader.read(buffer, 1);
            reader.close();
            return buffer[0];
        }
    }

    return 0;
}

char rduima::write(int address, char byte) {
    ofstream writer;
    writer.open(this->port);

    if (writer.is_open()) {
        writer << 0;
        writer << '\n';
        writer << address;
        writer << '\n';
        writer << byte;
        writer << '\n';
        writer.close();
    }

    ifstream reader;
    reader.open(this->port);
    char * buffer = new char[100];

    if (reader.is_open()) {
        while (buffer == 0) {
            reader.read(buffer, 1);
            reader.close();
            return buffer[0];
        }
    }
    return 0;
}

int main() {
    rduima test("/dev/cu.usbmodem1421");
    std << test.port << endl;
    return test.write(0x2A, 1);
}