#include "flight_state.h"
#include "time_utils.h"
#include "flight_result.h"
#include "result_storage.h"
#include <iostream>
#include <cstring>

void ProcessTelemetryPacket(const TelemetryPacket& packet, FlightState& state)
{
    std::cout << "[Telemetry Received] "
        << "Plane ID: " << packet.planeID
        << ", Timestamp: " << packet.timestamp
        << ", Remaining Fuel: " << packet.remainingFuel
        << ", End Of Flight: " << (packet.endOfFlight ? "true" : "false")
        << std::endl;

    if (!state.hasPreviousPacket)
    {
        state.previousPacket = packet;
        state.hasPreviousPacket = true;

        std::cout << "First telemetry record received for plane "
            << packet.planeID
            << ". Waiting for next packet to calculate fuel consumption."
            << std::endl;

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

    double currentConsumptionRate = fuelConsumed / elapsedSeconds;

    state.totalFuelConsumed += fuelConsumed;
    state.totalElapsedSeconds += elapsedSeconds;

    std::cout << "Current fuel consumed for plane "
        << packet.planeID
        << ": " << fuelConsumed << std::endl;

    std::cout << "Elapsed time since previous reading: "
        << elapsedSeconds << " seconds" << std::endl;

    std::cout << "Current fuel consumption rate for plane "
        << packet.planeID
        << ": " << currentConsumptionRate
        << " fuel units per second" << std::endl;

    state.previousPacket = packet;

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
    }
}