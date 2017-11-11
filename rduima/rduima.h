#include <string>

struct rduima {
    std::string port;
    rduima(std::string path) : port(path) {
    	std::cout << "HEY" << endl;
    }

    char read(int address);

    char write(int address, char byte);
};