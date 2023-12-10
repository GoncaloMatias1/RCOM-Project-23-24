#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include "url_parser.h"

int openSocket(char* ipAddress, int port);

int writeCommandToSocket(int fdSocket, const char* command);


int readSocketResponse(int fdSocket,char* response);