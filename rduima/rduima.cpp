#include "rduima.h"
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

char rduima::read(int address) {
    ofstream file;
    file.open(this->port);

    if (file.is_open()) {
        file << 0;
        file << '\n';
        file << address;
        file << '\n';
        file.close();
    }

    ifstream file;
    file.open(this->port);
    char * buffer = new char[100];

    if (file.is_open()) {
        while (buffer == 0) {
            file.read(buffer, 1);
            file.close();
            return buffer[0];
        }
    }

    return 0;
}

char rduima::write(int address, char byte) {
    ofstream file;
    file.open(this->port);

    if (file.is_open()) {
        file << 0;
        file << '\n';
        file << address;
        file << '\n';
        file << byte;
        file << '\n';
        file.close();
    }

    ifstream file;
    file.open(this->port);
    char * buffer = new char[LARGE_NUM];

    if (file.is_open()) {
        while (buffer == 0) {
            file.read(buffer, 1);
            file.close();
            return buffer[0];
        }
    }
    return 0;
}