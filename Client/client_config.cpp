#include "client_config.h"

#include <iostream>
#include <limits>
#include <string>

namespace
{
enum class OptionMatchResult
{
    NotMatched,
    Matched,
    Error
};

OptionMatchResult TryGetOptionValue(
    int argc,
    char* argv[],
    int& index,
    const std::string& argument,
    const std::string& optionName,
    std::string& value,
    std::string& errorMessage
)
{
    const std::string option = "--" + optionName;
    const std::string prefix = option + "=";

    if (argument.rfind(prefix, 0) == 0)
    {
        value = argument.substr(prefix.size());
        if (value.empty())
        {
            errorMessage = "Missing value for option " + option + ".";
            return OptionMatchResult::Error;
        }

        return OptionMatchResult::Matched;
    }

    if (argument == option)
    {
        if (index + 1 >= argc)
        {
            errorMessage = "Missing value for option " + option + ".";
            return OptionMatchResult::Error;
        }

        value = argv[++index];
        return OptionMatchResult::Matched;
    }

    return OptionMatchResult::NotMatched;
}

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

bool ParseDoubleValue(const std::string& text, double& value)
{
    std::size_t consumedCharacters = 0;

    try
    {
        value = std::stod(text, &consumedCharacters);
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

    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];

        if (argument == "--help" || argument == "-h")
        {
            showHelp = true;
            return true;
        }

        std::string value;

        OptionMatchResult result =
            TryGetOptionValue(argc, argv, index, argument, "host", value, errorMessage);
        if (result == OptionMatchResult::Error)
        {
            return false;
        }
        if (result == OptionMatchResult::Matched)
        {
            config.host = value;
            continue;
        }

        result = TryGetOptionValue(argc, argv, index, argument, "port", value, errorMessage);
        if (result == OptionMatchResult::Error)
        {
            return false;
        }
        if (result == OptionMatchResult::Matched)
        {
            unsigned long parsedPort = 0;
            if (!ParseUnsignedLongValue(value, parsedPort) || parsedPort == 0 || parsedPort > 65535)
            {
                errorMessage = "Invalid value for --port. Expected an integer between 1 and 65535.";
                return false;
            }

            config.port = static_cast<unsigned short>(parsedPort);
            continue;
        }

        result = TryGetOptionValue(argc, argv, index, argument, "plane-id", value, errorMessage);
        if (result == OptionMatchResult::Error)
        {
            return false;
        }
        if (result == OptionMatchResult::Matched)
        {
            unsigned long parsedPlaneID = 0;
            if (!ParseUnsignedLongValue(value, parsedPlaneID) ||
                parsedPlaneID > std::numeric_limits<unsigned int>::max())
            {
                errorMessage = "Invalid value for --plane-id. Expected a non-negative integer.";
                return false;
            }

            config.planeID = static_cast<unsigned int>(parsedPlaneID);
            continue;
        }

        result = TryGetOptionValue(argc, argv, index, argument, "delay-ms", value, errorMessage);
        if (result == OptionMatchResult::Error)
        {
            return false;
        }
        if (result == OptionMatchResult::Matched)
        {
            unsigned long parsedDelay = 0;
            if (!ParseUnsignedLongValue(value, parsedDelay) ||
                parsedDelay > std::numeric_limits<unsigned int>::max())
            {
                errorMessage = "Invalid value for --delay-ms. Expected a non-negative integer.";
                return false;
            }

            config.delayMs = static_cast<unsigned int>(parsedDelay);
            continue;
        }

        result = TryGetOptionValue(argc, argv, index, argument, "packet-count", value, errorMessage);
        if (result == OptionMatchResult::Error)
        {
            return false;
        }
        if (result == OptionMatchResult::Matched)
        {
            unsigned long parsedPacketCount = 0;
            if (!ParseUnsignedLongValue(value, parsedPacketCount) ||
                parsedPacketCount > std::numeric_limits<unsigned int>::max())
            {
                errorMessage = "Invalid value for --packet-count. Expected a positive integer.";
                return false;
            }

            config.packetCount = static_cast<unsigned int>(parsedPacketCount);
            continue;
        }

        result = TryGetOptionValue(argc, argv, index, argument, "start-fuel", value, errorMessage);
        if (result == OptionMatchResult::Error)
        {
            return false;
        }
        if (result == OptionMatchResult::Matched)
        {
            double parsedStartFuel = 0.0;
            if (!ParseDoubleValue(value, parsedStartFuel))
            {
                errorMessage = "Invalid value for --start-fuel. Expected a numeric value.";
                return false;
            }

            config.startFuel = parsedStartFuel;
            continue;
        }

        result = TryGetOptionValue(argc, argv, index, argument, "fuel-step", value, errorMessage);
        if (result == OptionMatchResult::Error)
        {
            return false;
        }
        if (result == OptionMatchResult::Matched)
        {
            double parsedFuelStep = 0.0;
            if (!ParseDoubleValue(value, parsedFuelStep))
            {
                errorMessage = "Invalid value for --fuel-step. Expected a numeric value.";
                return false;
            }

            config.fuelStep = parsedFuelStep;
            continue;
        }

        result = TryGetOptionValue(argc, argv, index, argument, "interval-seconds", value, errorMessage);
        if (result == OptionMatchResult::Error)
        {
            return false;
        }
        if (result == OptionMatchResult::Matched)
        {
            unsigned long parsedIntervalSeconds = 0;
            if (!ParseUnsignedLongValue(value, parsedIntervalSeconds) ||
                parsedIntervalSeconds > std::numeric_limits<unsigned int>::max())
            {
                errorMessage = "Invalid value for --interval-seconds. Expected a non-negative integer.";
                return false;
            }

            config.intervalSeconds = static_cast<unsigned int>(parsedIntervalSeconds);
            continue;
        }

        errorMessage = "Unknown option: " + argument;
        return false;
    }

    if (config.host.empty())
    {
        errorMessage = "The --host value cannot be empty.";
        return false;
    }

    if (config.fuelStep <= 0.0)
    {
        errorMessage = "The --fuel-step value must be greater than 0.";
        return false;
    }

    if (config.intervalSeconds == 0)
    {
        errorMessage = "The --interval-seconds value must be greater than 0.";
        return false;
    }

    if (config.packetCount < 2)
    {
        errorMessage = "The --packet-count value must be at least 2.";
        return false;
    }

    return true;
}

void PrintUsage(const char* executableName)
{
    std::cout
        << "Usage: " << executableName
        << " [--host value] [--port value] [--plane-id value]"
        << " [--delay-ms value] [--packet-count value]"
        << " [--start-fuel value] [--fuel-step value]"
        << " [--interval-seconds value]" << std::endl
        << std::endl
        << "Defaults:" << std::endl
        << "  --host=127.0.0.1" << std::endl
        << "  --port=18080" << std::endl
        << "  --plane-id=1001" << std::endl
        << "  --delay-ms=1000" << std::endl
        << "  --packet-count=5" << std::endl
        << "  --start-fuel=1000" << std::endl
        << "  --fuel-step=25" << std::endl
        << "  --interval-seconds=60" << std::endl
        << std::endl
        << "Example:" << std::endl
        << "  " << executableName
        << " --plane-id 2007 --packet-count 6 --fuel-step 30 --delay-ms 500"
        << std::endl;
}
