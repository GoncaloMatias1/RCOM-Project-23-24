#include "url_parser.h"

void initializeUrlInfo(urlInfo* url) {
    memset(url, 0, sizeof(urlInfo));
    url->port = 21;
}

char* getStringBeforeCharacter(const char* str, char chr) {
    const char* end = strchr(str, chr);
    if (end == NULL) {
        return NULL;
    }
    
    size_t length = end - str;
    char* buffer = malloc(length + 1);
    if (buffer == NULL) {
        perror("Malloc failed");
        return NULL;
    }

    strncpy(buffer, str, length);
    buffer[length] = '\0';
    return buffer;
}

int getIpAddressFromHost(urlInfo* url) {
    struct hostent* h = gethostbyname(url->host);
    if (h == NULL) {
        herror("gethostbyname failed");
        return -1;
    }

    struct in_addr* addr = (struct in_addr*)h->h_addr;
    if (addr == NULL) {
        fprintf(stderr, "No address found for host: %s\n", url->host);
        return -1;
    }

    strncpy(url->ipAddress, inet_ntoa(*addr), sizeof(url->ipAddress) - 1);
    url->ipAddress[sizeof(url->ipAddress) - 1] = '\0';
    return 0;
}

int parseUrlInfo(urlInfo* url, const char* urlGiven) {
    const char* protocol = "ftp://";
    if (strncmp(urlGiven, protocol, strlen(protocol)) != 0) {
        fprintf(stderr, "URL must start with '%s'\n", protocol);
        return -1;
    }

    const char* remaining = urlGiven + strlen(protocol);
    const char* atSymbol = strchr(remaining, '@');

    if (atSymbol) {
        const char* colon = strchr(remaining, ':');
        if (colon && colon < atSymbol) {
            size_t userLength = colon - remaining;
            size_t passLength = atSymbol - colon - 1;
            strncpy(url->user, remaining, userLength);
            url->user[userLength] = '\0';
            strncpy(url->password, colon + 1, passLength);
            url->password[passLength] = '\0';
        } else {
            fprintf(stderr, "Password not specified\n");
            return -1;
        }
        remaining = atSymbol + 1;  // Move past the '@' character
    } else {
        strcpy(url->user, "anonymous");
        strcpy(url->password, "anonymous@");
    }

    const char* slash = strchr(remaining, '/');
    if (!slash) {
        fprintf(stderr, "URL path not specified\n");
        return -1;
    }

    size_t hostLength = slash - remaining;
    strncpy(url->host, remaining, hostLength);
    url->host[hostLength] = '\0';
    strncpy(url->urlPath, slash + 1, sizeof(url->urlPath) - 1);

    const char* lastSlash = strrchr(url->urlPath, '/');
    if (lastSlash) {
        strncpy(url->fileName, lastSlash + 1, sizeof(url->fileName) - 1);
    } else {
        strncpy(url->fileName, url->urlPath, sizeof(url->fileName) - 1);
    }

    return 0;
}
