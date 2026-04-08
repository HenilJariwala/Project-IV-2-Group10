#ifndef SCENARIO_H
#define SCENARIO_H

#include "client_config.h"
#include "telemetry_packet.h"

#include <string>
#include <vector>

using FlightScenario = std::vector<TelemetryPacket>;

FlightScenario BuildScenario(const ClientConfig& config);
bool ValidateScenario(const FlightScenario& scenario, std::string& errorMessage);

#endif
