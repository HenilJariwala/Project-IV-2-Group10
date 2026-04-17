/**
 * @file client_handler.cpp
 * @brief Implements client connection handling and reliable telemetry packet reception.
 * @author Mohammad Aljabery
 */

#include "client_handler.h"
#include "telemetry_packet.h"
#include "flight_state.h"
#include <ws2tcpip.h>
#include <iostream>
#include <string>

namespace
{
    /**
     * @brief Receives an exact number of bytes from a client socket.
     * @param clientSocket The socket connected to the client.
     * @param buffer The buffer that stores the received data.
     * @param bufferSize The total number of bytes expected.
     * @param errorMessage A message describing the receive failure if one occurs.
     * @return true if the full buffer was received successfully, otherwise false.
     */
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

/**
 * @brief Handles communication with one connected aircraft client.
 * @param clientSocket The socket used to communicate with the client.
 * @param clientAddr The network address information of the connected client.
 */
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