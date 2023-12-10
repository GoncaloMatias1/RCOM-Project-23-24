#ifndef NETWORK_SOCKET_H
#define NETWORK_SOCKET_H

#include "url_parser.h" // For urlInfo type

int openSocket(const char* ipAddress, int port);
int writeCommandToSocket(int fdSocket, const char* command);
int readSocketResponse(int fdSocket, char* response);

#endif // NETWORK_SOCKET_H
