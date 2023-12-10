#include "socket_utils.h"
#define NETWORK_SOCKET_H
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#define RESPONSE_SIZE 1024



int createSocketConnection(char* ipAddress, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr)); // Use memset instead of bzero
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ipAddress);
    server_addr.sin_port = htons(port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        return -2;
    }

    return sockfd;
}


int transmitCommand(int fdSocket, const char* command) {
    if (command == NULL) {
        perror("Null command passed to transmitCommand\n");
        return -1;
    }

    ssize_t commandSize = strlen(command);
    ssize_t bytesWritten = write(fdSocket, command, commandSize);

    if (bytesWritten < 0) {
        perror("Error writing command to socket (write error)\n");
        return -2; // Differentiating write error
    }

    if (bytesWritten < commandSize) {
        fprintf(stderr, "Incomplete write: %zd out of %zd bytes written\n", bytesWritten, commandSize);
        return -3; // Differentiating incomplete write
    }

    return 0;
}

int socketOutput(int fdSocket, char *response) {
    if (response == NULL) {
        perror("Response buffer is NULL");
        return -1;
    }

    FILE* file = fdopen(fdSocket, "r");
    if (!file) {
        perror("fdopen() failed");
        return -2;
    }

    while (1) {
        memset(response, 0, RESPONSE_SIZE);
        if (!fgets(response, RESPONSE_SIZE, file)) {
            if (feof(file)) {
                printf("End of file reached on the socket\n");
                break; // Clean end of file
            } else {
                perror("fgets() failed");
                return -3; // fgets failed due to an error
            }
        }

        // Print the response directly
        printf("%s", response);

        // Check if the response line starts with a digit between '1' and '5' and the fourth character is a space
        if ('1' <= response[0] && response[0] <= '5' && response[3] == ' ') {
            break; // This is the last line of the response
        }
    }

    return 0;
}


