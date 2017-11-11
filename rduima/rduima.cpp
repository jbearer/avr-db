#include "rduima.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>     //String function definitions
#include <unistd.h>     //UNIX standard function definitions
#include <fcntl.h>      //File control definitions
#include <errno.h>      //Error number definitions
#include <termios.h>    //POSIX terminal control definitions

using namespace std;

fstream str;

char rduima::read(int address) {
    ofstream writer;
    writer.open(this->port);

    if (writer.is_open()) {
        writer << 1;
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
        reader.read(buffer, 1);
        reader.close();
        //cout << buffer[0] << endl;
        return buffer[0];
    }

    return 0;
}

char rduima::write(int address, int byte) {
    char * ch = new char[100];
    
    cout << str.is_open() << endl;
    if (str.is_open()) {
        usleep(2000000);
        str << '0' << endl;
        usleep(2000000);
        str << address << endl;
        usleep(2000000);
        str << byte << endl;
        usleep(2000000);
    }

    cout << str.is_open() << endl;
    if (str.is_open()) {
        //char c = '1';
        str.get(ch, 1);
        str.get(ch, 1);
        str.get(ch, 1);
        str.get(ch, 1);
        str.close();
        cout << ch << endl;
        return 0;
        //return buffer[0];
    }
    return 0;
}

int main() {
    rduima test("/dev/cu.usbmodem1421");
    str.open("/dev/cu.usbmodem1421");
    test.write(60, 2);
    //test.read(0x60);

    return 0;
}
