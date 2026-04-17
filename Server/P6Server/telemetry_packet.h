/**
 * @file telemetry_packet.h
 * @brief Defines the telemetry packet structure sent from the client to the server.
 * @author Mohammad Aljabery
 */

#ifndef TELEMETRY_PACKET_H
#define TELEMETRY_PACKET_H

 /**
  * @brief Represents one telemetry packet containing aircraft identification and fuel data.
  */
struct TelemetryPacket
{
    unsigned int planeID;
    char timestamp[32];
    double remainingFuel;
    bool endOfFlight;
};

#endif