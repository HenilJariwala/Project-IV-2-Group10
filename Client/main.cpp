#include "client_config.h"
#include "network_client.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    ClientConfig config;
    bool showHelp = false;
    std::string errorMessage;

    if (!ParseArguments(argc, argv, config, showHelp, errorMessage))
    {
        std::cerr << errorMessage << std::endl;
        PrintUsage((argc > 0 && argv[0] != nullptr) ? argv[0] : "client.exe");
        return 1;
    }

    if (showHelp)
    {
        PrintUsage((argc > 0 && argv[0] != nullptr) ? argv[0] : "client.exe");
        return 0;
    }

    return RunClient(config);
}
