#ifndef SCENARIO_H
#define SCENARIO_H

#include "telemetry_packet.h"

#include <string>
#include <vector>

using FlightScenario = std::vector<TelemetryPacket>;

bool LoadScenarioFromFile(
    const std::string& filePath,
    unsigned int planeID,
    FlightScenario& scenario,
    std::string& errorMessage
);

bool ValidateScenario(const FlightScenario& scenario, std::string& errorMessage);

#endif