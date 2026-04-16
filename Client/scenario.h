/**
 * @file scenario.h
 * @brief Declares functions for loading and validating telemetry scenarios.
 *
 * Defines the FlightScenario type and the interfaces used to read telemetry
 * data from files and verify that the generated scenario is valid.
 *
 * @author Nathan
 * @date 2026
 */

#ifndef SCENARIO_H
#define SCENARIO_H

#include "telemetry_packet.h"

#include <string>
#include <vector>

 /**
  * @brief Represents a sequence of telemetry packets for a single flight.
  */
using FlightScenario = std::vector<TelemetryPacket>;

/**
 * @brief Loads telemetry records from a file into a flight scenario.
 *
 * Parses telemetry lines, creates packets for the specified plane ID,
 * appends the required final end-of-flight packet, and validates the
 * completed scenario before returning it.
 *
 * @param filePath Path to the telemetry input file
 * @param planeID Plane ID assigned to all generated packets
 * @param scenario Output scenario containing parsed telemetry packets
 * @param errorMessage Output error message if loading fails
 * @return true if the scenario was loaded successfully, false otherwise
 */
bool LoadScenarioFromFile(
    const std::string& filePath,
    unsigned int planeID,
    FlightScenario& scenario,
    std::string& errorMessage
);

/**
 * @brief Validates the contents of a flight scenario.
 *
 * Checks packet count, timestamp format, packet ordering, and correct
 * use of the end-of-flight flag.
 *
 * @param scenario Scenario to validate
 * @param errorMessage Output error message if validation fails
 * @return true if the scenario is valid, false otherwise
 */
bool ValidateScenario(const FlightScenario& scenario, std::string& errorMessage);

#endif