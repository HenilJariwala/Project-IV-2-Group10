#include "scenario.h"

#include <cmath>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{
constexpr char TIMESTAMP_FORMAT[] = "%m_%d_%Y %H:%M:%S";
constexpr double FUEL_EPSILON = 1e-9;

bool ConvertToLocalTime(std::time_t timestamp, std::tm& localTime)
{
#ifdef _WIN32
    return localtime_s(&localTime, &timestamp) == 0;
#else
    return localtime_r(&timestamp, &localTime) != nullptr;
#endif
}

std::string FormatTimestamp(std::time_t timestamp)
{
    std::tm localTime = {};
    if (!ConvertToLocalTime(timestamp, localTime))
    {
        return "";
    }

    std::ostringstream output;
    output << std::put_time(&localTime, TIMESTAMP_FORMAT);
    return output.str();
}

bool ParseTimestampText(const char* timestampText, std::tm& parsedTime)
{
    parsedTime = {};
    parsedTime.tm_isdst = -1;

    std::istringstream input(timestampText);
    input >> std::get_time(&parsedTime, TIMESTAMP_FORMAT);

    return !input.fail();
}
}

FlightScenario BuildScenario(const ClientConfig& config)
{
    FlightScenario scenario;
    scenario.reserve(config.packetCount);

    const std::time_t startTimestamp = std::time(nullptr);

    for (unsigned int packetIndex = 0; packetIndex < config.packetCount; ++packetIndex)
    {
        TelemetryPacket packet{};
        packet.planeID = config.planeID;

        const std::time_t packetTimestamp =
            startTimestamp + static_cast<std::time_t>(packetIndex) * config.intervalSeconds;
        const std::string timestampText = FormatTimestamp(packetTimestamp);
        std::memcpy(packet.timestamp, timestampText.c_str(), timestampText.size());

        packet.remainingFuel =
            config.startFuel - static_cast<double>(packetIndex) * config.fuelStep;
        packet.endOfFlight = (packetIndex + 1 == config.packetCount);

        scenario.push_back(packet);
    }

    return scenario;
}

bool ValidateScenario(const FlightScenario& scenario, std::string& errorMessage)
{
    errorMessage.clear();

    if (scenario.size() < 2)
    {
        errorMessage = "The generated scenario must contain at least two telemetry packets.";
        return false;
    }

    for (std::size_t index = 0; index < scenario.size(); ++index)
    {
        const TelemetryPacket& packet = scenario[index];
        const std::size_t timestampLength = std::strlen(packet.timestamp);

        if (timestampLength == 0 || timestampLength >= sizeof(packet.timestamp))
        {
            errorMessage = "Packet " + std::to_string(index + 1) +
                " has an invalid timestamp length.";
            return false;
        }

        std::tm currentTime = {};
        if (!ParseTimestampText(packet.timestamp, currentTime))
        {
            errorMessage = "Packet " + std::to_string(index + 1) +
                " has a timestamp that does not match the server format.";
            return false;
        }

        if (index + 1 < scenario.size() && packet.endOfFlight)
        {
            errorMessage = "Only the final packet may set endOfFlight=true.";
            return false;
        }

        if (index + 1 == scenario.size() && !packet.endOfFlight)
        {
            errorMessage = "The final packet must set endOfFlight=true.";
            return false;
        }

        if (index > 0)
        {
            const TelemetryPacket& previousPacket = scenario[index - 1];
            if (packet.remainingFuel > previousPacket.remainingFuel + FUEL_EPSILON)
            {
                errorMessage = "Remaining fuel increased between packets " +
                    std::to_string(index) + " and " + std::to_string(index + 1) + ".";
                return false;
            }

            std::tm previousTime = {};
            if (!ParseTimestampText(previousPacket.timestamp, previousTime))
            {
                errorMessage = "Packet " + std::to_string(index) +
                    " has a timestamp that does not match the server format.";
                return false;
            }

            std::tm previousTimeCopy = previousTime;
            std::tm currentTimeCopy = currentTime;

            const std::time_t previousTimestampValue = std::mktime(&previousTimeCopy);
            const std::time_t currentTimestampValue = std::mktime(&currentTimeCopy);

            if (previousTimestampValue == static_cast<std::time_t>(-1) ||
                currentTimestampValue == static_cast<std::time_t>(-1) ||
                std::difftime(currentTimestampValue, previousTimestampValue) <= 0.0)
            {
                errorMessage = "Packet timestamps must be strictly increasing.";
                return false;
            }
        }
    }

    return true;
}
