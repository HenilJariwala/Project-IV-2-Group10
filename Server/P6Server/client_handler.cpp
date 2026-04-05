#include "client_handler.h"
#include "telemetry_packet.h"
#include "flight_state.h"
#include <ws2tcpip.h>
#include <iostream>

void HandleClient(SOCKET clientSocket, sockaddr_in clientAddr)
{
    char clientIp[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);

    std::cout << "Aircraft client is being handled in a new thread: "
        << clientIp << ":" << ntohs(clientAddr.sin_port) << std::endl;

    FlightState flightState;

    while (true)
    {
        TelemetryPacket packet{};

        int bytesReceived = recv(
            clientSocket,
            reinterpret_cast<char*>(&packet),
            sizeof(TelemetryPacket),
            0
        );

        if (bytesReceived == 0)
        {
            std::cout << "Plane landed like butter: "
                << clientIp << ":" << ntohs(clientAddr.sin_port) << std::endl;
            break;
        }

        if (bytesReceived == SOCKET_ERROR)
        {
            std::cerr << "recv() failed for client "
                << clientIp << ":" << ntohs(clientAddr.sin_port)
                << ". Error code: " << WSAGetLastError() << std::endl;
            break;
        }

        if (bytesReceived != sizeof(TelemetryPacket))
        {
            std::cerr << "Partial or invalid packet received from client "
                << clientIp << ":" << ntohs(clientAddr.sin_port)
                << ". Expected " << sizeof(TelemetryPacket)
                << " bytes but got " << bytesReceived << " bytes." << std::endl;
            break;
        }

        ProcessTelemetryPacket(packet, flightState);

        if (packet.endOfFlight)
        {
            break;
        }
    }

    closesocket(clientSocket);

    std::cout << "Client socket closed: "
        << clientIp << ":" << ntohs(clientAddr.sin_port) << std::endl;
}