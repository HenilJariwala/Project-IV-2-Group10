#ifndef TELEMETRY_PACKET_H
#define TELEMETRY_PACKET_H

#include <cstddef>
#include <type_traits>

struct TelemetryPacket
{
    unsigned int planeID;
    char timestamp[32];
    double remainingFuel;
    bool endOfFlight;
};

#endif
