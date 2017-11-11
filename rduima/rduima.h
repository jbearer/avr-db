#include <string>

struct rduima {
    std::string port;
    rduima(std::string path) : port(path) {

    }

    char read(uint address);

    void write(uint address, char byte);
}