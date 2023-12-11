#include "socket_utils.h"
#define NETWORK_SOCKET_H
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#define RESPONSE_SIZE 1024

int createSocketConnection(char* ipAddress, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ipAddress);
    server_addr.sin_port = htons(port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed to create socket");
        return -1;
    }
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection attempt failed");
        return -2;
    }

    return sockfd;
}

int transmitCommand(int fdSocket, const char* command) {
    if (command == NULL) {
        perror("Command not provided to transmitCommand");
        return -1;
    }

    ssize_t commandSize = strlen(command);
    ssize_t bytesWritten = write(fdSocket, command, commandSize);

    if (bytesWritten < 0) {
        perror("Unable to write command to socket");
        return -2;
    }

    if (bytesWritten < commandSize) {
        fprintf(stderr, "Partial write: only %zd of %zd bytes transmitted", bytesWritten, commandSize);
        return -3;
    }

    return 0;
}

int socketOutput(int fdSocket, char *response) {
    if (response == NULL) {
        perror("Invalid response buffer");
        return -1;
    }

    FILE* file = fdopen(fdSocket, "r");
    if (!file) {
        perror("Unable to open file descriptor");
        return -2;
    }

    while (1) {
        memset(response, 0, RESPONSE_SIZE);
        if (!fgets(response, RESPONSE_SIZE, file)) {
            if (feof(file)) {
                printf("Reached end of file on socket");
                break;
            } else {
                perror("Failed to read from socket");
                return -3;
            }
        }

        printf("%s", response);

        if ('1' <= response[0] && response[0] <= '5' && response[3] == ' ') {
            break;
        }
    }

    return 0;
}
