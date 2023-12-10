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
        return; // It's always a good practice to check for NULL pointers
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

    url->port = 21; // Default FTP port
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
        fprintf(stderr, "Invalid URL or host information\n");
        return -1;
    }

    struct hostent* h;

    if ((h = gethostbyname(url->host)) == NULL) {
        herror("gethostbyname");
        return -1;
    }

    // Convert the first address found to a string
    if (inet_ntop(h->h_addrtype, h->h_addr, url->ipAddress, IP_ADDRESS_LENGTH) == NULL) {
        perror("inet_ntop");
        return -1;
    }

    return 0;
}


int urlParsing(urlInfo* url, char* urlGiven) {
    const char* protocolPrefix = "ftp://";
    const size_t protocolLength = 6; // Length of "ftp://"

    // Check if the URL starts with "ftp://"
    if (strncmp(urlGiven, protocolPrefix, protocolLength) != 0) {
        fprintf(stderr, "URL does not start with 'ftp://'\n");
        return -1;
    }

    // Allocate memory for auxiliary URL and URL path
    size_t urlLength = strlen(urlGiven);
    char* auxUrl = malloc(urlLength); // No +1 for '\0' because we are not copying the initial "ftp://"
    char* urlPath = malloc(urlLength);

    if (!auxUrl || !urlPath) {
        fprintf(stderr, "Failed to allocate memory for URL parsing\n");
        free(auxUrl); // Safe to call free on NULL
        free(urlPath); // Safe to call free on NULL
        return -1;
    }

    // Skip the "ftp://" part
    strcpy(auxUrl, urlGiven + protocolLength);

    // Split the URL into user and path
    char* atSignPtr = strchr(auxUrl, '@');
    if (atSignPtr) {
        strcpy(urlPath, atSignPtr + 1); // Copy path part
        *atSignPtr = '\0'; // Split the string into user and path

        char* colonPtr = strchr(auxUrl, ':');
        if (colonPtr) {
            *colonPtr = '\0'; // Split the string into user and password
            strcpy(url->password, colonPtr + 1);
            strcpy(url->user, auxUrl);
        } else {
            strcpy(url->user, auxUrl);
            // Handle password input separately
        }
    } else {
        strcpy(urlPath, auxUrl);
        strcpy(url->user, "anonymous");
        strcpy(url->password, "anypassword");
    }

    // Split the path into host and URL path
    char* slashPtr = strchr(urlPath, '/');
    if (slashPtr) {
        *slashPtr = '\0'; // Split the string into host and URL path
        strcpy(url->host, urlPath);
        strcpy(url->urlPath, slashPtr + 1);
    } else {
        strcpy(url->host, urlPath);
        // Handle cases where the URL path is absent
    }

    // Extract the filename from the URL path
    char* lastSlashPtr = strrchr(url->urlPath, '/');
    if (lastSlashPtr) {
        strcpy(url->fileName, lastSlashPtr + 1);
    } else {
        strcpy(url->fileName, url->urlPath);
    }

    // Clean up
    free(auxUrl);
    free(urlPath);

    return 0;
}
