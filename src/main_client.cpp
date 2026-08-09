#include <cstdint>
#include <iostream>

#include "Client.h"
#include "Logger.h"

int main(int argc, char* argv[]) {
    std::string address = (argc > 1) ? argv[1] : "127.0.0.1";
    uint16_t port = (argc > 2) ? static_cast<uint16_t>(std::stoi(argv[2])) : 54000;

    netconnect::Logger logger("logs/client.log");
    netconnect::Client client(address, port, logger);
    client.run();

    return 0;
}
