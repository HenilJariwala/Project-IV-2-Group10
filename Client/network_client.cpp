#include "network_client.h"

#include "scenario.h"

#include <ws2tcpip.h>

#include <array>
#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif

namespace
{
    constexpr unsigned int PACKET_DELAY_MS = 1000;

    const std::array<std::string, 4> TELEMETRY_FILES =
    {
        "katl-kefd-B737-700.txt",
        "Telem_2023_3_12 14_56_40.txt",
        "Telem_2023_3_12 16_26_4.txt",
        "Telem_czba-cykf-pa28-w2_2023_3_1 12_31_27.txt"
    };

    std::mutex g_consoleMutex;
    std::mutex g_randomMutex;
    std::mutex g_scenarioCacheMutex;
    std::unordered_map<std::string, FlightScenario> g_scenarioCache;

    void LogLine(const std::string& text)
    {
        std::lock_guard<std::mutex> lock(g_consoleMutex);
        std::cout << text << std::endl;
    }

    std::string PickRandomTelemetryFile()
    {
        static std::mt19937 generator(static_cast<unsigned int>(
            std::chrono::steady_clock::now().time_since_epoch().count()
            ));

        std::lock_guard<std::mutex> lock(g_randomMutex);
        std::uniform_int_distribution<int> distribution(0, static_cast<int>(TELEMETRY_FILES.size() - 1));
        return TELEMETRY_FILES[distribution(generator)];
    }

    const FlightScenario* GetCachedScenario(
        const std::string& fileName,
        std::string& errorMessage)
    {
        {
            std::lock_guard<std::mutex> lock(g_scenarioCacheMutex);
            const auto it = g_scenarioCache.find(fileName);
            if (it != g_scenarioCache.end())
            {
                return &it->second;
            }
        }

        FlightScenario loadedScenario;
        constexpr unsigned int CACHE_PLANE_ID = 0;

        if (!LoadScenarioFromFile(fileName, CACHE_PLANE_ID, loadedScenario, errorMessage))
        {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(g_scenarioCacheMutex);
        auto [it, inserted] = g_scenarioCache.emplace(fileName, std::move(loadedScenario));
        return &it->second;
    }

    void RunSinglePlaneClient(const ClientConfig& baseConfig, unsigned int planeID)
    {
        ClientConfig planeConfig = baseConfig;
        const std::string fileName = PickRandomTelemetryFile();

        LogLine("Plane " + std::to_string(planeID) + " selected file: " + fileName);

        std::string errorMessage;
        const FlightScenario* scenario = GetCachedScenario(fileName, errorMessage);

        if (scenario == nullptr)
        {
            LogLine("Plane " + std::to_string(planeID) + " failed to load scenario: " + errorMessage);
            return;
        }

        LogLine(
            "Plane " + std::to_string(planeID) +
            " reused cached scenario with " + std::to_string(scenario->size()) +
            " packets."
        );

        SOCKET clientSocket = ConnectToServer(planeConfig, errorMessage);
        if (clientSocket == INVALID_SOCKET)
        {
            LogLine("Plane " + std::to_string(planeID) + " failed to connect: " + errorMessage);
            return;
        }

        LogLine("Plane " + std::to_string(planeID) + " connected successfully.");

        for (std::size_t packetIndex = 0; packetIndex < scenario->size(); ++packetIndex)
        {
            TelemetryPacket packet = (*scenario)[packetIndex];
            packet.planeID = planeID;

            LogLine(
                "Plane " + std::to_string(planeID) +
                " | Sending packet " + std::to_string(packetIndex + 1) +
                "/" + std::to_string(scenario->size())
            );

            int bytesWritten = 0;
            if (!SendPacket(clientSocket, packet, bytesWritten, errorMessage))
            {
                LogLine(
                    "Plane " + std::to_string(planeID) +
                    " failed to send packet " + std::to_string(packetIndex + 1) +
                    ": " + errorMessage
                );

                shutdown(clientSocket, SD_BOTH);
                closesocket(clientSocket);
                return;
            }

            if (packetIndex + 1 < scenario->size())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(PACKET_DELAY_MS));
            }
        }

        shutdown(clientSocket, SD_SEND);
        closesocket(clientSocket);

        LogLine(
            "Plane " + std::to_string(planeID) +
            " finished sending " + std::to_string(scenario->size()) +
            " packets and disconnected."
        );
    }
}

SOCKET ConnectToServer(const ClientConfig& config, std::string& errorMessage)
{
    errorMessage.clear();

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
    LogLine("Initializing Winsock for client launcher...");

    WSADATA wsaData{};
    const int startupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (startupResult != 0)
    {
        LogLine("WSAStartup failed with error code: " + std::to_string(startupResult));
        return 1;
    }

    LogLine(
        "Winsock initialized successfully. Launching plane IDs " +
        std::to_string(config.startPlaneID) + " to " + std::to_string(config.endPlaneID) + "."
    );

    std::vector<std::thread> clientThreads;
    clientThreads.reserve(static_cast<std::size_t>(config.endPlaneID - config.startPlaneID + 1));

    for (unsigned int planeID = config.startPlaneID; planeID <= config.endPlaneID; ++planeID)
    {
        clientThreads.emplace_back(RunSinglePlaneClient, config, planeID);
    }

    for (std::thread& clientThread : clientThreads)
    {
        if (clientThread.joinable())
        {
            clientThread.join();
        }
    }

    WSACleanup();
    LogLine("All client threads finished. Winsock cleaned up.");

    return 0;
}
