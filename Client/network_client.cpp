#include "network_client.h"

#include "scenario.h"

#include <ws2tcpip.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif

namespace
{
void PrintConfiguration(const ClientConfig& config)
{
    std::cout << "Client configuration:" << std::endl;
    std::cout << "  Host: " << config.host << std::endl;
    std::cout << "  Port: " << config.port << std::endl;
    std::cout << "  Plane ID: " << config.planeID << std::endl;
    std::cout << "  Delay (ms): " << config.delayMs << std::endl;
    std::cout << "  Packet count: " << config.packetCount << std::endl;
    std::cout << "  Start fuel: " << config.startFuel << std::endl;
    std::cout << "  Fuel step: " << config.fuelStep << std::endl;
    std::cout << "  Interval seconds: " << config.intervalSeconds << std::endl;
}

void PrintPacketDetails(const TelemetryPacket& packet, std::size_t packetIndex, std::size_t packetCount)
{
    std::cout << "Sending packet " << packetIndex << "/" << packetCount
        << " | Plane ID: " << packet.planeID
        << " | Timestamp: " << packet.timestamp
        << " | Remaining Fuel: " << packet.remainingFuel
        << " | End Of Flight: " << (packet.endOfFlight ? "true" : "false")
        << std::endl;
}
}

SOCKET ConnectToServer(const ClientConfig& config, std::string& errorMessage)
{
    errorMessage.clear();
    std::cout << "Connecting to " << config.host << ":" << config.port << "..." << std::endl;

    addrinfo addr{};
    addr.ai_family = AF_INET;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = IPPROTO_TCP;

    const std::string portText = std::to_string(config.port);
    addrinfo* addressList = nullptr;

    const int addressResult =
        getaddrinfo(config.host.c_str(), portText.c_str(), &addr, &addressList);
    if (addressResult != 0)
    {
        errorMessage = "getaddrinfo() failed with error code: " + std::to_string(addressResult);
        return INVALID_SOCKET;
    }

    SOCKET clientSocket = INVALID_SOCKET;
    int lastSocketError = 0;

    for (addrinfo* currentAddress = addressList;
         currentAddress != nullptr;
         currentAddress = currentAddress->ai_next)
    {
        clientSocket = socket(
            currentAddress->ai_family,
            currentAddress->ai_socktype,
            currentAddress->ai_protocol
        );

        if (clientSocket == INVALID_SOCKET)
        {
            lastSocketError = WSAGetLastError();
            continue;
        }

        BOOL noDelay = TRUE;
        if (setsockopt(
                clientSocket,
                IPPROTO_TCP,
                TCP_NODELAY,
                reinterpret_cast<const char*>(&noDelay),
                sizeof(noDelay)
            ) == SOCKET_ERROR)
        {
            lastSocketError = WSAGetLastError();
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
            continue;
        }

        if (connect(
                clientSocket,
                currentAddress->ai_addr,
                static_cast<int>(currentAddress->ai_addrlen)
            ) == SOCKET_ERROR)
        {
            lastSocketError = WSAGetLastError();
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
            continue;
        }

        break;
    }

    freeaddrinfo(addressList);

    if (clientSocket == INVALID_SOCKET)
    {
        errorMessage = "Unable to connect to the server. Last error code: " +
            std::to_string(lastSocketError);
        return INVALID_SOCKET;
    }

    std::cout << "Connected to the server successfully." << std::endl;
    return clientSocket;
}

bool SendPacket(
    SOCKET clientSocket,
    const TelemetryPacket& packet,
    int& bytesWritten,
    std::string& errorMessage
)
{
    errorMessage.clear();
    bytesWritten = 0;

    const char* packetBytes = reinterpret_cast<const char*>(&packet);
    const int packetSize = static_cast<int>(sizeof(TelemetryPacket));

    while (bytesWritten < packetSize)
    {
        const int bytesSent = send(
            clientSocket,
            packetBytes + bytesWritten,
            packetSize - bytesWritten,
            0
        );

        if (bytesSent == SOCKET_ERROR)
        {
            errorMessage = "send() failed with error code: " +
                std::to_string(WSAGetLastError());
            return false;
        }

        if (bytesSent == 0)
        {
            errorMessage = "send() returned 0 before the packet was fully written.";
            return false;
        }

        bytesWritten += bytesSent;
    }

    return true;
}

int RunClient(const ClientConfig& config)
{
    PrintConfiguration(config);

    FlightScenario scenario = BuildScenario(config);
    std::string errorMessage;
    if (!ValidateScenario(scenario, errorMessage))
    {
        std::cerr << "Scenario validation failed: " << errorMessage << std::endl;
        return 1;
    }

    std::cout << "Generated " << scenario.size()
        << " telemetry packets. Initializing Winsock..." << std::endl;

    WSADATA wsaData{};
    const int startupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (startupResult != 0)
    {
        std::cerr << "WSAStartup failed with error code: " << startupResult << std::endl;
        return 1;
    }

    std::cout << "Winsock initialized successfully." << std::endl;

    SOCKET clientSocket = ConnectToServer(config, errorMessage);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << errorMessage << std::endl;
        WSACleanup();
        return 1;
    }

    for (std::size_t packetIndex = 0; packetIndex < scenario.size(); ++packetIndex)
    {
        const TelemetryPacket& packet = scenario[packetIndex];
        PrintPacketDetails(packet, packetIndex + 1, scenario.size());

        int bytesWritten = 0;
        if (!SendPacket(clientSocket, packet, bytesWritten, errorMessage))
        {
            std::cerr << "Failed to send packet " << (packetIndex + 1)
                << ": " << errorMessage << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "Packet " << (packetIndex + 1)
            << " sent successfully (" << bytesWritten << " bytes)." << std::endl;

        if (packetIndex + 1 < scenario.size())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(config.delayMs));
        }
    }

    closesocket(clientSocket);
    std::cout << "Disconnected from the server." << std::endl;

    WSACleanup();
    std::cout << "Winsock cleaned up. Flight transmission complete." << std::endl;

    return 0;
}
