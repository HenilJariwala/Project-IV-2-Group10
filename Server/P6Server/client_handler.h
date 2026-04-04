#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <winsock2.h>

void HandleClient(SOCKET clientSocket, sockaddr_in clientAddr);

#endif