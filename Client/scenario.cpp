/**
 * @file scenario.cpp
 * @brief Implements telemetry scenario parsing and validation.
 *
 * Provides helper functions for trimming text, splitting input lines,
 * parsing numeric and timestamp values, building telemetry packets,
 * loading scenarios from files, and validating packet order and format.
 *
 * @author Nathan
 * @date 2026
 */
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
    // Expected timestamp format used by telemetry records and server validation
    constexpr char TIMESTAMP_FORMAT[] = "%m_%d_%Y %H:%M:%S";

    /**
     * @brief Removes leading and trailing whitespace from a string.
     *
     * @param text Input text
     * @return Trimmed string
     */
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

    /**
     * @brief Splits a comma-separated line into trimmed tokens.
     *
     * @param line Input telemetry line
     * @return Vector of trimmed tokens
     */
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

    /**
     * @brief Converts a string into a double value.
     *
     * Ensures that the entire input string is consumed during conversion.
     *
     * @param text Input text
     * @param value Parsed numeric value
     * @return true if conversion succeeds, false otherwise
     */
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

    /**
     * @brief Parses a timestamp string into a tm structure.
     *
     * @param timestampText Timestamp text to parse
     * @param parsedTime Output parsed time structure
     * @return true if parsing succeeds, false otherwise
     */
    bool ParseTimestampText(const char* timestampText, std::tm& parsedTime)
    {
        parsedTime = {};
        parsedTime.tm_isdst = -1;

        std::istringstream input(timestampText);
        input >> std::get_time(&parsedTime, TIMESTAMP_FORMAT);

        return !input.fail();
    }

    /**
     * @brief Builds a telemetry packet from a single input line.
     *
     * Extracts timestamp and fuel values from the telemetry line, validates
     * them, and fills the packet for the specified plane ID.
     *
     * @param line Input telemetry line
     * @param planeID Plane ID to assign to the packet
     * @param packet Output telemetry packet
     * @param errorMessage Output error message if parsing fails
     * @return true if the packet was built successfully, false otherwise
     */
    bool BuildPacketFromLine(
        const std::string& line,
        unsigned int planeID,
        TelemetryPacket& packet,
        std::string& errorMessage
    )
    {
        // Break the telemetry line into trimmed comma-separated fields
        const std::vector<std::string> tokens = SplitByComma(line);

        std::string timestampText;
        std::string fuelText;

        // Support both labeled and unlabeled telemetry line formats
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

        // Ensure the timestamp fits into the fixed-size packet buffer
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

        // Clear packet contents before assigning parsed telemetry values
        packet = {};
        packet.planeID = planeID;
        std::memcpy(packet.timestamp, timestampText.c_str(), timestampText.size());
        packet.remainingFuel = remainingFuel;
        packet.endOfFlight = false;

        return true;
    }
}

/**
 * @brief Reads telemetry data from a file and constructs a valid scenario.
 *
 * Skips empty lines, parses each telemetry record into a packet, appends
 * the required final packet with endOfFlight=true, and validates the
 * completed scenario before returning it.
 */
bool LoadScenarioFromFile(
    const std::string& filePath,
    unsigned int planeID,
    FlightScenario& scenario,
    std::string& errorMessage
)
{
    scenario.clear();
    errorMessage.clear();

    // Open the telemetry file for sequential line-by-line parsing
    std::ifstream inputFile(filePath);
    if (!inputFile.is_open())
    {
        errorMessage = "Failed to open telemetry file: " + filePath;
        return false;
    }

    std::string line;
    unsigned int lineNumber = 0;

    // Read and parse each non-empty telemetry record from the file
    while (std::getline(inputFile, line))
    {
        ++lineNumber;

        // Ignore blank lines so only real telemetry records are processed
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

    // Reject files that do not contain any valid telemetry records
    if (scenario.empty())
    {
        errorMessage = "Telemetry file contains no valid telemetry records: " + filePath;
        return false;
    }

    // Require at least two real telemetry packets before adding the final marker packet
    if (scenario.size() < 2)
    {
        errorMessage = "Telemetry file must contain at least two real telemetry records: " + filePath;
        return false;
    }
    
    // Duplicate the last real packet and mark it as the final end-of-flight packet
    TelemetryPacket finalPacket = scenario.back();
    finalPacket.endOfFlight = true;
    scenario.push_back(finalPacket);

    return ValidateScenario(scenario, errorMessage);
}

/**
 * @brief Verifies that a scenario satisfies packet count, timestamp, and ordering rules.
 *
 * Ensures that timestamps are valid and non-decreasing, only the final
 * packet uses endOfFlight=true, and the scenario contains the required
 * minimum number of packets.
 */
bool ValidateScenario(const FlightScenario& scenario, std::string& errorMessage)
{
    errorMessage.clear();

    // A valid scenario must include at least two real packets and one final packet
    if (scenario.size() < 3)
    {
        errorMessage = "Scenario must contain at least two real packets and one final end-of-flight packet.";
        return false;
    }

    // Validate each packet's timestamp format and end-of-flight state
    for (std::size_t index = 0; index < scenario.size(); ++index)
    {
        const TelemetryPacket& packet = scenario[index];
        const std::size_t timestampLength = std::strlen(packet.timestamp);

        // Reject packets with missing or oversized timestamp text
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

        // Only the extra final packet is allowed to mark the end of flight
        if (index + 1 < scenario.size() - 1 && packet.endOfFlight)
        {
            errorMessage = "Only the extra final packet may set endOfFlight=true.";
            return false;
        }

        // Ensure the last packet is explicitly marked as the end-of-flight packet
        if (index + 1 == scenario.size() && !packet.endOfFlight)
        {
            errorMessage = "The final packet must set endOfFlight=true.";
            return false;
        }

        // Compare each packet timestamp with the previous one to ensure correct order
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

            // Convert parsed timestamps into comparable time values
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
                // Reject scenarios where packet timestamps move backwards in time
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
