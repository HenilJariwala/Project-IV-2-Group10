/*
Author: Mohammad Aljabrery but this is a team effort really
Purpose: Project 2 server - this is just the basic TCP startup and listening socket
*/

#include "server.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

int main()
{
    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    sockaddr_in serverAddr{};

    int port = SERVER_PORT;

    std::cout << "Starting server..." << std::endl;

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

    //keep the server running for now
    std::cout << "do Ctrl+C to stop the server for now" << std::endl;
    while (true)
    {
        Sleep(1000);
    }


    closesocket(listenSocket);
    WSACleanup();

    return 0;
}