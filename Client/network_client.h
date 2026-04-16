/**
 * @file network_client.h
 * @brief Declares networking functions for client connection and packet transmission.
 *
 * Contains the interface for connecting to the server, sending telemetry
 * packets, and running the client launcher for a range of plane IDs.
 *
 * @author Henil
 * @date 2026
 */

#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include "client_config.h"
#include "telemetry_packet.h"

#include <string>
#include <winsock2.h>

 /**
  * @brief Establishes a TCP connection to the server.
  *
  * Uses the host and port values from the client configuration to resolve
  * the server address and attempt a socket connection.
  *
  * @param config Client configuration containing server connection details
  * @param errorMessage Output error message if connection fails
  * @return Connected socket on success, or INVALID_SOCKET on failure
  */
SOCKET ConnectToServer(const ClientConfig& config, std::string& errorMessage);

/**
 * @brief Sends a telemetry packet over an established socket connection.
 *
 * Ensures that the complete packet is transmitted even if multiple send
 * calls are required.
 *
 * @param clientSocket Connected client socket
 * @param packet Telemetry packet to send
 * @param bytesWritten Total number of bytes sent
 * @param errorMessage Output error message if sending fails
 * @return true if the full packet was sent successfully, false otherwise
 */
bool SendPacket(
    SOCKET clientSocket,
    const TelemetryPacket& packet,
    int& bytesWritten,
    std::string& errorMessage
);

/**
 * @brief Launches client threads for the configured plane ID range.
 *
 * Initializes Winsock, starts one client thread per plane ID, waits for
 * all threads to finish, and then cleans up networking resources.
 *
 * @param config Client configuration containing plane ID range and server details
 * @return 0 on success, non-zero on initialization failure
 */
int RunClient(const ClientConfig& config);

#endif
