#include <string>

struct rduima {
    string port;
    rduima(string path) : port(path) {

    }

    char read(uint address);

    void write(uint address, char byte);
}