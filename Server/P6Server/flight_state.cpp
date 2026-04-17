/**
 * @file flight_state.cpp
 * @brief Implements telemetry packet processing and fuel consumption calculations for each flight.
 * @author Mohammad Aljabery
 */

#include "flight_state.h"
#include "time_utils.h"
#include "flight_result.h"
#include "result_storage.h"
#include <iostream>
#include <cstring>

 /**
  * @brief Processes a telemetry packet and updates the flight state for an aircraft.
  * @param packet The telemetry packet received from the client.
  * @param state The flight state being updated for the aircraft.
  */
void ProcessTelemetryPacket(const TelemetryPacket& packet, FlightState& state)
{
    std::cout << "Plane " << packet.planeID
        << " | Fuel Remaining: " << packet.remainingFuel
        << std::endl;

    if (!state.hasPreviousPacket)
    {
        state.previousPacket = packet;
        state.hasPreviousPacket = true;

        return;
    }

    std::tm previousTime = {};
    std::tm currentTime = {};

    bool previousParsed = ParseTimestamp(state.previousPacket.timestamp, previousTime);
    bool currentParsed = ParseTimestamp(packet.timestamp, currentTime);

    if (!previousParsed || !currentParsed)
    {
        std::cerr << "Failed to parse timestamp for plane "
            << packet.planeID << std::endl;

        state.previousPacket = packet;
        return;
    }

    if (packet.endOfFlight)
    {
        std::cout << "End of flight received for plane "
            << packet.planeID << std::endl;

        if (state.totalElapsedSeconds > 0.0)
        {
            double averageFuelConsumptionRate =
                state.totalFuelConsumed / state.totalElapsedSeconds;

            std::cout << "Final average fuel consumption rate for plane "
                << packet.planeID
                << ": " << averageFuelConsumptionRate
                << " fuel units per second" << std::endl;

            FlightResult result;
            result.planeID = packet.planeID;
            result.averageFuelConsumptionRate = averageFuelConsumptionRate;

            SaveFlightResult(result);
        }
        else
        {
            std::cout << "Not enough telemetry data to calculate final average fuel consumption for plane "
                << packet.planeID << std::endl;
        }

        state.previousPacket = packet;
        return;
    }

    double elapsedSeconds = GetSecondsBetween(previousTime, currentTime);
    double fuelConsumed = state.previousPacket.remainingFuel - packet.remainingFuel;

    if (elapsedSeconds <= 0.0)
    {
        std::cerr << "Invalid timestamp order for plane "
            << packet.planeID
            << ". Elapsed seconds: " << elapsedSeconds << std::endl;

        state.previousPacket = packet;
        return;
    }

    if (fuelConsumed < 0.0)
    {
        std::cerr << "Fuel increased between readings for plane "
            << packet.planeID
            << ". Skipping this interval." << std::endl;

        state.previousPacket = packet;
        return;
    }

    state.totalFuelConsumed += fuelConsumed;
    state.totalElapsedSeconds += elapsedSeconds;
    state.previousPacket = packet;
}