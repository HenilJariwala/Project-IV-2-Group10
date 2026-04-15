#include "client_handler.h"
#include "telemetry_packet.h"
#include "flight_state.h"
#include <ws2tcpip.h>
#include <iostream>
#include <string>

namespace
{
    bool ReceiveAll(SOCKET clientSocket, char* buffer, int bufferSize, std::string& errorMessage)
    {
        int totalBytesReceived = 0;

        while (totalBytesReceived < bufferSize)
        {
            const int bytesReceived = recv(
                clientSocket,
                buffer + totalBytesReceived,
                bufferSize - totalBytesReceived,
                0
            );

            if (bytesReceived == 0)
            {
                errorMessage = "Client disconnected before a full packet was received.";
                return false;
            }

            if (bytesReceived == SOCKET_ERROR)
            {
                errorMessage = "recv() failed with error code: " + std::to_string(WSAGetLastError());
                return false;
            }

            totalBytesReceived += bytesReceived;
        }

        return true;
    }
}

void HandleClient(SOCKET clientSocket, sockaddr_in clientAddr)
{
    char clientIp[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);

    std::cout << "Aircraft client is being handled in a worker thread: "
        << clientIp << ":" << ntohs(clientAddr.sin_port) << std::endl;

    FlightState flightState;

    while (true)
    {
        TelemetryPacket packet{};
        std::string errorMessage;

        if (!ReceiveAll(
            clientSocket,
            reinterpret_cast<char*>(&packet),
            static_cast<int>(sizeof(TelemetryPacket)),
            errorMessage))
        {
            std::cout << "Client " << clientIp << ":" << ntohs(clientAddr.sin_port)
                << " disconnected or failed receive: " << errorMessage << std::endl;
            break;
        }
        ProcessTelemetryPacket(packet, flightState);

        if (packet.endOfFlight)
        {
            break;
        }
    }

    shutdown(clientSocket, SD_BOTH);
    closesocket(clientSocket);

    flightState = FlightState{};

    std::cout << "Client socket closed: "
        << clientIp << ":" << ntohs(clientAddr.sin_port) << std::endl;
}