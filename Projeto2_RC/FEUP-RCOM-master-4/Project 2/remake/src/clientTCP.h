// clientTCP.h
#ifndef CLIENTTCP_H
#define CLIENTTCP_H

#include "getip.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>

int openSocket(char* ipAddress, int port);
int writeCommandToSocket(int fdSocket, char* command);
int readSocketResponse(int fdSocket, char* response);

int ftpStartConnection(int* fdSocket, urlInfo* url);
int ftpLoginIn(urlInfo* url, int fdSocket);
int ftpPassiveMode(urlInfo* url, int fdSocket, int* dataSocket);
int ftpRetrieveFile(urlInfo* url, int fdSocket, int * fileSize);
int ftpDownloadAndCreateFile(urlInfo* url, int fdDataSocket, int fileSize);

#endif /* CLIENTTCP_H */
