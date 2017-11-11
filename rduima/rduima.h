#include <string>

struct rduima {
    std::string port;
    rduima(std::string path) : port(path) {

    }

    char read(int address);

    void write(int address, char byte);
};