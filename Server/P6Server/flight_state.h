/**
 * @file flight_state.h
 * @brief Defines the flight state structure and telemetry processing function used by the server.
 * @author Mohammad Aljabery
 */

#ifndef FLIGHT_STATE_H
#define FLIGHT_STATE_H

#include "telemetry_packet.h"

 /**
  * @brief Stores telemetry history and accumulated fuel usage data for one flight.
  */
struct FlightState
{
    bool hasPreviousPacket = false;
    TelemetryPacket previousPacket{};
    double totalFuelConsumed = 0.0;
    double totalElapsedSeconds = 0.0;
};

/**
 * @brief Processes a received telemetry packet and updates the flight state.
 * @param packet The telemetry packet received from the client.
 * @param state The flight state associated with the aircraft.
 */
void ProcessTelemetryPacket(const TelemetryPacket& packet, FlightState& state);

#endif