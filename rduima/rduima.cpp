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

    //cout << reader.is_open() << endl;
    if (reader.is_open()) {
        while (buffer == 0) {
            reader.read(buffer, 1);
            reader.close();
            //cout << buffer[0] << endl;
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

    cout << reader.is_open() << endl;
    if (reader.is_open()) {
        while (buffer == 0) {
            reader.read(buffer, 1);
            reader.close();
            cout << buffer[0] << endl;
            return buffer[0];
        }
    }
    return 0;
}

int main() {
    rduima test("/dev/cu.usbmodem1421");
    test.write(0x60, '1');
    test.read(0x60);

    return 0;
}