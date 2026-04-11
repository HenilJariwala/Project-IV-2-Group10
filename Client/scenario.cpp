#include "scenario.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    constexpr char TIMESTAMP_FORMAT[] = "%m_%d_%Y %H:%M:%S";

    std::string Trim(const std::string& text)
    {
        std::size_t start = 0;
        while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])))
        {
            ++start;
        }

        std::size_t end = text.size();
        while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])))
        {
            --end;
        }

        return text.substr(start, end - start);
    }

    std::vector<std::string> SplitByComma(const std::string& line)
    {
        std::vector<std::string> tokens;
        std::stringstream input(line);
        std::string part;

        while (std::getline(input, part, ','))
        {
            tokens.push_back(Trim(part));
        }

        return tokens;
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

    bool ParseTimestampText(const char* timestampText, std::tm& parsedTime)
    {
        parsedTime = {};
        parsedTime.tm_isdst = -1;

        std::istringstream input(timestampText);
        input >> std::get_time(&parsedTime, TIMESTAMP_FORMAT);

        return !input.fail();
    }

    bool BuildPacketFromLine(
        const std::string& line,
        unsigned int planeID,
        TelemetryPacket& packet,
        std::string& errorMessage
    )
    {
        const std::vector<std::string> tokens = SplitByComma(line);

        std::string timestampText;
        std::string fuelText;

        if (tokens.size() >= 3 && tokens[0] == "FUEL TOTAL QUANTITY")
        {
            timestampText = tokens[1];
            fuelText = tokens[2];
        }
        else if (tokens.size() >= 2)
        {
            timestampText = tokens[0];
            fuelText = tokens[1];
        }
        else
        {
            errorMessage = "Line does not contain enough comma-separated values.";
            return false;
        }

        if (timestampText.empty())
        {
            errorMessage = "Timestamp is empty.";
            return false;
        }

        if (fuelText.empty())
        {
            errorMessage = "Fuel value is empty.";
            return false;
        }

        if (timestampText.size() >= sizeof(packet.timestamp))
        {
            errorMessage = "Timestamp is too long for TelemetryPacket.timestamp.";
            return false;
        }

        double remainingFuel = 0.0;
        if (!ParseDoubleValue(fuelText, remainingFuel))
        {
            errorMessage = "Fuel value is not numeric: " + fuelText;
            return false;
        }

        packet = {};
        packet.planeID = planeID;
        std::memcpy(packet.timestamp, timestampText.c_str(), timestampText.size());
        packet.remainingFuel = remainingFuel;
        packet.endOfFlight = false;

        return true;
    }
}

bool LoadScenarioFromFile(
    const std::string& filePath,
    unsigned int planeID,
    FlightScenario& scenario,
    std::string& errorMessage
)
{
    scenario.clear();
    errorMessage.clear();

    std::ifstream inputFile(filePath);
    if (!inputFile.is_open())
    {
        errorMessage = "Failed to open telemetry file: " + filePath;
        return false;
    }

    std::string line;
    unsigned int lineNumber = 0;

    while (std::getline(inputFile, line))
    {
        ++lineNumber;

        if (Trim(line).empty())
        {
            continue;
        }

        TelemetryPacket packet{};
        std::string lineError;

        if (!BuildPacketFromLine(line, planeID, packet, lineError))
        {
            errorMessage = "File " + filePath + ", line " + std::to_string(lineNumber) +
                ": " + lineError;
            return false;
        }

        scenario.push_back(packet);
    }

    if (scenario.empty())
    {
        errorMessage = "Telemetry file contains no valid telemetry records: " + filePath;
        return false;
    }

    if (scenario.size() < 2)
    {
        errorMessage = "Telemetry file must contain at least two real telemetry records: " + filePath;
        return false;
    }

    TelemetryPacket finalPacket = scenario.back();
    finalPacket.endOfFlight = true;
    scenario.push_back(finalPacket);

    return ValidateScenario(scenario, errorMessage);
}

bool ValidateScenario(const FlightScenario& scenario, std::string& errorMessage)
{
    errorMessage.clear();

    if (scenario.size() < 3)
    {
        errorMessage = "Scenario must contain at least two real packets and one final end-of-flight packet.";
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

        if (index + 1 < scenario.size() - 1 && packet.endOfFlight)
        {
            errorMessage = "Only the extra final packet may set endOfFlight=true.";
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
                currentTimestampValue == static_cast<std::time_t>(-1))
            {
                errorMessage = "Failed to convert packet timestamps to time values.";
                return false;
            }

            if (index + 1 < scenario.size())
            {
                if (std::difftime(currentTimestampValue, previousTimestampValue) < 0.0)
                {
                    errorMessage = "Packet timestamps must not go backwards.";
                    return false;
                }
            }
        }
    }

    const TelemetryPacket& secondLastPacket = scenario[scenario.size() - 2];
    const TelemetryPacket& finalPacket = scenario.back();

    return true;
}
