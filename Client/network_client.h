#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include "client_config.h"
#include "telemetry_packet.h"

#include <string>
#include <winsock2.h>

SOCKET ConnectToServer(const ClientConfig& config, std::string& errorMessage);
bool SendPacket(
    SOCKET clientSocket,
    const TelemetryPacket& packet,
    int& bytesWritten,
    std::string& errorMessage
);
int RunClient(const ClientConfig& config);

#endif
