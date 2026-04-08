#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <string>

struct ClientConfig
{
    std::string host = "127.0.0.1";
    unsigned short port = 18080;
    unsigned int planeID = 1001;
    unsigned int delayMs = 1000;
    unsigned int packetCount = 5;
    double startFuel = 1000.0;
    double fuelStep = 25.0;
    unsigned int intervalSeconds = 60;
};

bool ParseArguments(
    int argc,
    char* argv[],
    ClientConfig& config,
    bool& showHelp,
    std::string& errorMessage
);

void PrintUsage(const char* executableName);

#endif
