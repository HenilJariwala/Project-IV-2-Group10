/**
 * @file network_client.cpp
 * @brief Implements client networking, telemetry transmission, and multi-client execution.
 *
 * Provides logic for selecting telemetry files, caching parsed scenarios,
 * connecting to the server, sending packets, and running one client thread
 * for each plane ID.
 *
 * @author Henil
 * @date 2026
 */

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
    // Delay between consecutive packet transmissions to simulate real-time streaming
    constexpr unsigned int PACKET_DELAY_MS = 1000;

    // List of telemetry input files available for random client selection
    const std::array<std::string, 4> TELEMETRY_FILES =
    {
        "katl-kefd-B737-700.txt",
        "Telem_2023_3_12 14_56_40.txt",
        "Telem_2023_3_12 16_26_4.txt",
        "Telem_czba-cykf-pa28-w2_2023_3_1 12_31_27.txt"
    };

    // Shared synchronization objects used for console output, random selection,
    // and cached scenario access across multiple client threads
    std::mutex g_consoleMutex;
    std::mutex g_randomMutex;
    std::mutex g_scenarioCacheMutex;
    std::unordered_map<std::string, FlightScenario> g_scenarioCache;

    /**
     * @brief Writes a thread-safe log message to the console.
     *
     * @param text Message to display
     */
    void LogLine(const std::string& text)
    {
        std::lock_guard<std::mutex> lock(g_consoleMutex);
        std::cout << text << std::endl;
    }

    /**
     * @brief Selects a telemetry file at random from the available file list.
     *
     * Uses a shared random number generator protected by a mutex to ensure
     * thread-safe file selection.
     *
     * @return Name of the selected telemetry file
     */
    std::string PickRandomTelemetryFile()
    {
        static std::mt19937 generator(static_cast<unsigned int>(
            std::chrono::steady_clock::now().time_since_epoch().count()
            ));

        std::lock_guard<std::mutex> lock(g_randomMutex);
        std::uniform_int_distribution<int> distribution(0, static_cast<int>(TELEMETRY_FILES.size() - 1));
        return TELEMETRY_FILES[distribution(generator)];
    }

    /**
     * @brief Retrieves a parsed flight scenario from cache or loads it from file.
     *
     * If the scenario has already been loaded, the cached version is reused.
     * Otherwise, the scenario is parsed from disk and stored for future reuse.
     *
     * @param fileName Name of the telemetry file
     * @param errorMessage Output error message if loading fails
     * @return Pointer to the cached scenario, or nullptr on failure
     */
    const FlightScenario* GetCachedScenario(
        const std::string& fileName,
        std::string& errorMessage)
    {
        {
            std::lock_guard<std::mutex> lock(g_scenarioCacheMutex);
            const auto it = g_scenarioCache.find(fileName); // Check whether this telemetry file has already been parsed and cached
            if (it != g_scenarioCache.end())
            {
                return &it->second;
            }
        }

        // Load the scenario from file only if it is not already available in cache
        FlightScenario loadedScenario;
        constexpr unsigned int CACHE_PLANE_ID = 0;

        if (!LoadScenarioFromFile(fileName, CACHE_PLANE_ID, loadedScenario, errorMessage))
        {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(g_scenarioCacheMutex);
        // Store the newly loaded scenario so future clients can reuse it
        auto [it, inserted] = g_scenarioCache.emplace(fileName, std::move(loadedScenario));
        return &it->second;
    }

    /**
     * @brief Runs a single client instance for one plane ID.
     *
     * Selects a telemetry file, retrieves its parsed scenario, connects to the
     * server, sends all packets for the assigned plane, and then closes the socket.
     *
     * @param baseConfig Base client configuration
     * @param planeID Plane ID assigned to this client instance
     */
    void RunSinglePlaneClient(const ClientConfig& baseConfig, unsigned int planeID)
    {
        ClientConfig planeConfig = baseConfig; // Create a per-plane configuration instance for this client thread
        const std::string fileName = PickRandomTelemetryFile();

        LogLine("Plane " + std::to_string(planeID) + " selected file: " + fileName);

        std::string errorMessage;
        const FlightScenario* scenario = GetCachedScenario(fileName, errorMessage);

        // Stop this client if the telemetry scenario could not be loaded
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
        // Abort this client if a server connection could not be established
        if (clientSocket == INVALID_SOCKET)
        {
            LogLine("Plane " + std::to_string(planeID) + " failed to connect: " + errorMessage);
            return;
        }

        LogLine("Plane " + std::to_string(planeID) + " connected successfully.");
        // Send each telemetry packet in sequence for the current plane
        for (std::size_t packetIndex = 0; packetIndex < scenario->size(); ++packetIndex)
        {
            TelemetryPacket packet = (*scenario)[packetIndex];
            // Assign the current plane ID before transmitting the packet
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
                // Pause between packets to preserve simulated telemetry timing
                std::this_thread::sleep_for(std::chrono::milliseconds(PACKET_DELAY_MS));
            }
        }

        // Close the sending side of the connection after all packets are transmitted
        shutdown(clientSocket, SD_SEND);
        closesocket(clientSocket);

        LogLine(
            "Plane " + std::to_string(planeID) +
            " finished sending " + std::to_string(scenario->size()) +
            " packets and disconnected."
        );
    }
}

/**
 * @brief Resolves the server address and attempts to open a TCP connection.
 *
 * Tries each resolved address until a connection succeeds or all options fail.
 * Also enables TCP_NODELAY on the connected socket.
 */
SOCKET ConnectToServer(const ClientConfig& config, std::string& errorMessage)
{
    errorMessage.clear();

    // Prepare address lookup hints for an IPv4 TCP connection
    addrinfo addr{};
    addr.ai_family = AF_INET;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = IPPROTO_TCP;

    const std::string portText = std::to_string(config.port);
    addrinfo* addressList = nullptr;

    // Resolve the server host and port into one or more usable socket addresses
    const int addressResult =
        getaddrinfo(config.host.c_str(), portText.c_str(), &addr, &addressList);
    if (addressResult != 0)
    {
        errorMessage = "getaddrinfo() failed with error code: " + std::to_string(addressResult);
        return INVALID_SOCKET;
    }

    SOCKET clientSocket = INVALID_SOCKET;
    int lastSocketError = 0;

    // Try each resolved address until a working socket connection is established
    for (addrinfo* currentAddress = addressList;
        currentAddress != nullptr;
        currentAddress = currentAddress->ai_next)
    {
        // Create a socket for the current candidate server address
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

        // Disable Nagle's algorithm to send packets without extra buffering delay
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

        // Attempt to connect using the current resolved address
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

    // Release memory allocated by getaddrinfo after connection attempts are complete
    freeaddrinfo(addressList);

    if (clientSocket == INVALID_SOCKET)
    {
        errorMessage = "Unable to connect to the server. Last error code: " +
            std::to_string(lastSocketError);
        return INVALID_SOCKET;
    }

    return clientSocket;
}

/**
 * @brief Sends a full telemetry packet over the socket.
 *
 * Repeats send operations until the entire packet has been written or an error occurs.
 */
bool SendPacket(
    SOCKET clientSocket,
    const TelemetryPacket& packet,
    int& bytesWritten,
    std::string& errorMessage
)
{
    errorMessage.clear();
    bytesWritten = 0;

    // Treat the packet as a raw byte buffer for transmission over the socket
    const char* packetBytes = reinterpret_cast<const char*>(&packet);
    const int packetSize = static_cast<int>(sizeof(TelemetryPacket));

    // Keep sending until the complete packet has been transmitted
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

/**
 * @brief Starts all client threads for the configured plane range.
 *
 * Initializes Winsock once, launches one thread per plane ID, waits for all
 * clients to complete, and then performs cleanup.
 */
int RunClient(const ClientConfig& config)
{
    LogLine("Initializing Winsock for client launcher...");

    // Initialize the Winsock library before creating any client sockets
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

    // Store all client threads so they can be joined before shutdown
    std::vector<std::thread> clientThreads;
    clientThreads.reserve(static_cast<std::size_t>(config.endPlaneID - config.startPlaneID + 1));

    // Launch one client thread for each plane ID in the configured range
    for (unsigned int planeID = config.startPlaneID; planeID <= config.endPlaneID; ++planeID)
    {
        clientThreads.emplace_back(RunSinglePlaneClient, config, planeID);
    }

    // Wait for all client threads to finish before cleaning up networking resources
    for (std::thread& clientThread : clientThreads)
    {
        if (clientThread.joinable())
        {
            clientThread.join();
        }
    }

    // Release Winsock resources after all client work is complete
    WSACleanup();
    LogLine("All client threads finished. Winsock cleaned up.");

    return 0;
}
