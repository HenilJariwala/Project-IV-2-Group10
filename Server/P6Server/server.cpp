/**
 * @file server.cpp
 * @brief Implements the main multi-threaded telemetry server, including socket setup, worker thread management, and client connection acceptance.
 * @author Mohammad Aljabery
 */

 /*
 Author: Mohammad Aljabrery and improved by Henil Jariwala
 Purpose: Project 2 server
 */

#include <algorithm>
#include <chrono>
#include "server.h"
#include "client_handler.h"
#include <winsock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

namespace
{

    /**
     * @brief Represents a pending client connection waiting to be processed by a worker thread.
     */
    struct ClientJob
    {
        SOCKET socket = INVALID_SOCKET;
        sockaddr_in address{};
    };

    std::mutex g_queueMutex;
    std::condition_variable g_queueCondition;
    std::queue<ClientJob> g_pendingClients;

    std::mutex g_workerMutex;
    std::vector<std::thread> g_workers;
    unsigned int g_activeWorkers = 0;

    constexpr unsigned int MIN_WORKERS = 4;
    constexpr unsigned int MAX_WORKERS = 64;
    constexpr auto WORKER_IDLE_TIMEOUT = std::chrono::seconds(15);

    /**
     * @brief Joins and removes worker threads that have already finished execution.
     */
    void CleanupFinishedWorkers()
    {
        std::lock_guard<std::mutex> lock(g_workerMutex);

        auto it = g_workers.begin();
        while (it != g_workers.end())
        {
            if (it->joinable())
            {
                DWORD result = WaitForSingleObject(
                    reinterpret_cast<HANDLE>(it->native_handle()),
                    0
                );

                if (result == WAIT_OBJECT_0)
                {
                    it->join();
                    it = g_workers.erase(it);
                    continue;
                }
            }

            ++it;
        }
    }


    /**
     * @brief Waits for pending client jobs and processes them using a worker thread.
     */
    void WorkerLoop()
    {
        while (true)
        {
            ClientJob job{};

            {
                std::unique_lock<std::mutex> lock(g_queueMutex);

                bool gotJob = g_queueCondition.wait_for(
                    lock,
                    WORKER_IDLE_TIMEOUT,
                    [] { return !g_pendingClients.empty(); }
                );

                if (!gotJob)
                {
                    if (g_activeWorkers > MIN_WORKERS)
                    {
                        --g_activeWorkers;
                        std::cout << "Idle worker exiting. Active workers: "
                            << g_activeWorkers << std::endl;
                        return;
                    }

                    continue;
                }

                job = g_pendingClients.front();
                g_pendingClients.pop();
            }

            HandleClient(job.socket, job.address);
        }
    }

    void EnsureWorkerCapacity()
    {
        std::lock_guard<std::mutex> queueLock(g_queueMutex);

        const std::size_t pendingCount = g_pendingClients.size();

        if (pendingCount > g_activeWorkers && g_activeWorkers < MAX_WORKERS)
        {
            unsigned int workersToAdd =
                static_cast<unsigned int>(
                    std::min<std::size_t>(pendingCount - g_activeWorkers, MAX_WORKERS - g_activeWorkers)
                    );

            std::lock_guard<std::mutex> workerLock(g_workerMutex);

            for (unsigned int i = 0; i < workersToAdd; ++i)
            {
                g_workers.emplace_back(WorkerLoop);
                ++g_activeWorkers;
            }

            std::cout << "Scaled worker pool. Active workers: "
                << g_activeWorkers << std::endl;
        }
    }
}



/**
 * @brief Initializes the server, starts the worker thread pool, accepts incoming client connections,
 *        and dispatches connected clients for telemetry processing.
 * @author Mohammad Aljabery
 * @return Returns 0 if the server shuts down normally, or 1 if initialization, socket creation,
 *         binding, or listening fails.
 */
int main()
{
    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    sockaddr_in serverAddr{};

    int port = SERVER_PORT;

    std::cout << "=============================" << std::endl;
    std::cout << "     Starting server..." << std::endl;
    std::cout << "=============================" << std::endl;


    //initailize Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup has failed. With error code: " << result << std::endl;
        return 1;
    }
    std::cout << "Winsock has been initialized successfully. \\o/ " << std::endl;

    //create the listening socket
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        std::cerr << "Socket creation has failed. With error code: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    std::cout << "Listening socket has been created successfully. \\o/ " << std::endl;

    BOOL reuseAddress = TRUE;
    setsockopt(
        listenSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(&reuseAddress),
        sizeof(reuseAddress)
    );

    //configure the server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    //bind the socket to the port
    result = bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
    if (result == SOCKET_ERROR)
    {
        std::cerr << "Binding failed. With error code: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Socket has been bound to port " << port << ". \\o/ " << std::endl;

    //start listening
    result = listen(listenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR)
    {
        std::cerr << "Listen failed. Wih error code: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Control tower is now scanning for planes..." << std::endl;
    std::cout << "do Ctrl+C to stop the server for now" << std::endl;

    {
        std::lock_guard<std::mutex> workerLock(g_workerMutex);
        for (unsigned int index = 0; index < MIN_WORKERS; ++index)
        {
            g_workers.emplace_back(WorkerLoop);
            ++g_activeWorkers;
        }
    }

    std::cout << "Started " << MIN_WORKERS << " base worker threads." << std::endl;

    while (true)
    {
        SOCKET clientSocket = INVALID_SOCKET;
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);

        std::cout << "Waiting for an incoming client connectioon..." << std::endl;

        clientSocket = accept(
            listenSocket,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientAddrSize
        );

        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "accept() failed. With error code: " << WSAGetLastError() << std::endl;
            continue;
        }
        
        {
            std::lock_guard<std::mutex> lock(g_queueMutex);
            g_pendingClients.push(ClientJob{ clientSocket, clientAddr });
        }

        EnsureWorkerCapacity();
        g_queueCondition.notify_one();
        CleanupFinishedWorkers();
    }

    closesocket(listenSocket);
    WSACleanup();

    return 0;
}