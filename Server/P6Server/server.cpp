/*
Author: Mohammad Aljabrery but this is a team effort really
Purpose: Project 2 server
*/

#include "server.h"
#include "client_handler.h"
#include <winsock2.h>
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
    struct ClientJob
    {
        SOCKET socket = INVALID_SOCKET;
        sockaddr_in address{};
    };

    std::mutex g_queueMutex;
    std::condition_variable g_queueCondition;
    std::queue<ClientJob> g_pendingClients;

    void WorkerLoop()
    {
        while (true)
        {
            ClientJob job;

            {
                std::unique_lock<std::mutex> lock(g_queueMutex);
                g_queueCondition.wait(lock, [] { return !g_pendingClients.empty(); });
                job = g_pendingClients.front();
                g_pendingClients.pop();
            }

            HandleClient(job.socket, job.address);
        }
    }
}

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

    unsigned int workerCount = std::thread::hardware_concurrency();
    if (workerCount == 0)
    {
        workerCount = 4;
    }

    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    for (unsigned int index = 0; index < workerCount; ++index)
    {
        workers.emplace_back(WorkerLoop);
    }

    std::cout << "Started " << workerCount << " reusable worker threads." << std::endl;

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
        g_queueCondition.notify_one();
    }

    for (std::thread& worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }

    closesocket(listenSocket);
    WSACleanup();

    return 0;
}