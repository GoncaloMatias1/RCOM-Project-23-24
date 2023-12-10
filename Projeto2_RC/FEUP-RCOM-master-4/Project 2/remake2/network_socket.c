#include "network_socket.h"
#include <unistd.h>

int openSocket(const char* ipAddress, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ipAddress);
    server_addr.sin_port = htons(port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket() failed");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed");
        close(sockfd); // Ensure we close the socket if connect fails
        return -1;
    }

    return sockfd;
}

int writeCommandToSocket(int fdSocket, const char* command) {
    ssize_t commandSize = strlen(command);
    ssize_t bytesWritten = write(fdSocket, command, commandSize);

    if (bytesWritten < commandSize) {
        perror("write() to socket failed");
        return -1;
    }

    return 0;
}

int readSocketResponse(int fdSocket, char* response) {
    FILE* file = fdopen(fdSocket, "r");
    if (!file) {
        perror("fdopen() failed");
        return -1;
    }

    char* result = NULL;
    do {
        result = fgets(response, 1024, file);
        if (result == NULL) {
            perror("fgets() failed");
            fclose(file);
            return -1;
        }
        printf("%s", response);
    } while (!('1' <= response[0] && response[0] <= '5') || response[3] != ' ');

    fclose(file);
    return 0;
}
