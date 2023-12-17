#include "parsing_url.h"

#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#define URL_INFO_FIELD_SIZE 256
#define URL_INFO_NUM_FIELDS 6
#define IP_ADDRESS_LENGTH INET_ADDRSTRLEN

void urlConfig(urlInfo* url){
    if (url == NULL) {
        return;
    }

    char* urlFields[URL_INFO_NUM_FIELDS] = {
        url->user,
        url->password,
        url->host,
        url->urlPath,
        url->ipAddress,
        url->fileName
    };

    for (int i = 0; i < URL_INFO_NUM_FIELDS; i++) {
        memset(urlFields[i], 0, URL_INFO_FIELD_SIZE);
    }

    url->port = 21;
}

char* extractTextBefore(char* str, char chr){
    char* aux = (char*)malloc(strlen(str));
    strcpy(aux, strchr(str, chr));
    int index = strlen(str) - strlen(aux);

    aux[index] = '\0';

    strncpy(aux,str,index);

    return aux;
}

int getHostIP(urlInfo* url) {
    if (url == NULL || url->host == NULL) {
        fprintf(stderr, "URL or host details are missing\n");
        return -1;
    }

    struct hostent* h;

    if ((h = gethostbyname(url->host)) == NULL) {
        herror("Host name resolution failed");
        return -1;
    }

    if (inet_ntop(h->h_addrtype, h->h_addr, url->ipAddress, IP_ADDRESS_LENGTH) == NULL) {
        perror("IP address conversion error");
        return -1;
    }

    return 0;
}

int urlParsing(urlInfo* url, char* urlGiven) {
    const char* protocolPrefix = "ftp://";
    const size_t protocolLength = 6;

    if (strncmp(urlGiven, protocolPrefix, protocolLength) != 0) {
        fprintf(stderr, "URL must begin with 'ftp://'\n");
        return -1;
    }

    size_t urlLength = strlen(urlGiven);
    char* auxUrl = malloc(urlLength); 
    char* urlPath = malloc(urlLength);

    if (!auxUrl || !urlPath) {
        fprintf(stderr, "Memory allocation for URL parsing failed\n");
        free(auxUrl);
        free(urlPath);
        return -1;
    }

    strcpy(auxUrl, urlGiven + protocolLength);

    char* atSignPtr = strchr(auxUrl, '@');
    if (atSignPtr) {
        strcpy(urlPath, atSignPtr + 1);
        *atSignPtr = '\0';

        char* colonPtr = strchr(auxUrl, ':');
        if (colonPtr) {
            *colonPtr = '\0';
            strcpy(url->password, colonPtr + 1);
            strcpy(url->user, auxUrl);
        } else {
            strcpy(url->user, auxUrl);
        }
    } else {
        strcpy(urlPath, auxUrl);
        strcpy(url->user, "anonymous");
        strcpy(url->password, "anypassword");
    }

    char* slashPtr = strchr(urlPath, '/');
    if (slashPtr) {
        *slashPtr = '\0';
        strcpy(url->host, urlPath);
        strcpy(url->urlPath, slashPtr + 1);
    } else {
        strcpy(url->host, urlPath);
    }

    char* lastSlashPtr = strrchr(url->urlPath, '/');
    if (lastSlashPtr) {
        strcpy(url->fileName, lastSlashPtr + 1);
    } else {
        strcpy(url->fileName, url->urlPath);
    }

    free(auxUrl);
    free(urlPath);

    return 0;
}
