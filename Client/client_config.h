#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <string>

struct ClientConfig
{
    std::string host;
    unsigned short port = 18080;
    unsigned int startPlaneID = 0;
    unsigned int endPlaneID = 0;
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
