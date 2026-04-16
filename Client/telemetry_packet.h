/**
 * @file telemetry_packet.h
 * @brief Defines the telemetry packet structure used for network transmission.
 *
 * Contains the TelemetryPacket structure, which represents a single unit
 * of telemetry data exchanged between client and server.
 *
 * @author Nathan
 * @date 2026
 */

#ifndef TELEMETRY_PACKET_H
#define TELEMETRY_PACKET_H

#include <cstddef>
#include <type_traits>

 /**
  * @struct TelemetryPacket
  * @brief Represents a single telemetry data packet.
  *
  * Stores plane identification, timestamp, remaining fuel, and a flag
  * indicating the end of a flight sequence.
  */
struct TelemetryPacket
{
    unsigned int planeID;      // Unique identifier for the plane
    char timestamp[32];        // Timestamp string (fixed-size buffer)
    double remainingFuel;      // Remaining fuel value at this timestamp
    bool endOfFlight;          // Indicates if this is the final packet
};

#endif
