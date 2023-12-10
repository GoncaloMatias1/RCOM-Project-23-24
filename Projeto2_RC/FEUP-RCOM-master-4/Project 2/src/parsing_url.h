#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h>

#define h_addr h_addr_list[0]


typedef struct {
    char user[256];
    char password[256];
    char host[256];
    char urlPath[256];
    char ipAddress[256];
    char fileName[256];
    int port;

}urlInfo;

void urlConfig(urlInfo* url);

char* extractTextBefore(char* str, char chr);

int getHostIP(urlInfo* url);

int urlParsing(urlInfo* url, char * urlGiven);