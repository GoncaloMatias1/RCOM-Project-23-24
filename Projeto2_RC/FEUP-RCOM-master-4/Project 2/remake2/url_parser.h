#ifndef URL_PARSER_H
#define URL_PARSER_H

#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h>

typedef struct {
    char user[256];
    char password[256];
    char host[256];
    char urlPath[256];
    char ipAddress[256];
    char fileName[256];
    int port;
} urlInfo;

void initializeUrlInfo(urlInfo* url);
char* getStringBeforeCharacther(char* str, char chr);
int getIpAddressFromHost(urlInfo* url);
int parseUrlInfo(urlInfo* url, const char* urlGiven);

#endif // URL_PARSER_H
