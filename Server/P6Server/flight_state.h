#ifndef FLIGHT_STATE_H
#define FLIGHT_STATE_H

#include "telemetry_packet.h"

struct FlightState
{
    bool hasPreviousPacket = false;
    TelemetryPacket previousPacket{};
    double totalFuelConsumed = 0.0;
    double totalElapsedSeconds = 0.0;
};

void ProcessTelemetryPacket(const TelemetryPacket& packet, FlightState& state);

#endif