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
#include "parsing_url.h"

int createSocketConnection(char* ipAddress, int port);

int transmitCommand(int fdSocket, const char* command);


int socketOutput(int fdSocket,char* response);