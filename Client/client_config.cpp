#include "client_config.h"

#include <iostream>
#include <limits>
#include <string>

namespace
{
    bool ParseUnsignedLongValue(const std::string& text, unsigned long& value)
    {
        std::size_t consumedCharacters = 0;

        try
        {
            value = std::stoul(text, &consumedCharacters);
        }
        catch (...)
        {
            return false;
        }

        return consumedCharacters == text.size();
    }
}

bool ParseArguments(
    int argc,
    char* argv[],
    ClientConfig& config,
    bool& showHelp,
    std::string& errorMessage
)
{
    showHelp = false;
    errorMessage.clear();

    if (argc >= 2)
    {
        const std::string firstArgument = argv[1];
        if (firstArgument == "--help" || firstArgument == "-h")
        {
            showHelp = true;
            return true;
        }
    }

    if (argc != 4)
    {
        errorMessage = "Expected exactly 3 arguments: ServerIP startID endID";
        return false;
    }

    config.host = argv[1];

    if (config.host.empty())
    {
        errorMessage = "ServerIP cannot be empty.";
        return false;
    }

    unsigned long parsedStartID = 0;
    if (!ParseUnsignedLongValue(argv[2], parsedStartID) ||
        parsedStartID > std::numeric_limits<unsigned int>::max())
    {
        errorMessage = "Invalid startID. Expected a non-negative integer.";
        return false;
    }

    unsigned long parsedEndID = 0;
    if (!ParseUnsignedLongValue(argv[3], parsedEndID) ||
        parsedEndID > std::numeric_limits<unsigned int>::max())
    {
        errorMessage = "Invalid endID. Expected a non-negative integer.";
        return false;
    }

    config.startPlaneID = static_cast<unsigned int>(parsedStartID);
    config.endPlaneID = static_cast<unsigned int>(parsedEndID);

    if (config.startPlaneID == 0)
    {
        errorMessage = "startID must be at least 1.";
        return false;
    }

    if (config.endPlaneID < config.startPlaneID)
    {
        errorMessage = "endID must be greater than or equal to startID.";
        return false;
    }

    return true;
}

void PrintUsage(const char* executableName)
{
    std::cout
        << "Usage: " << executableName << " ServerIP startID endID" << std::endl
        << std::endl
        << "Example:" << std::endl
        << "  " << executableName << " localhost 1 20" << std::endl
        << std::endl
        << "This will launch one client thread per plane ID in the given range." << std::endl
        << "Each client chooses one telemetry file randomly, opens its own TCP connection," << std::endl
        << "sends all real telemetry lines, then sends one extra final packet with endOfFlight=true." << std::endl;
}
