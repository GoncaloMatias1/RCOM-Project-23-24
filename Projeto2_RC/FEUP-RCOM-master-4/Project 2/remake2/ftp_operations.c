#include <unistd.h> // for close()
#include <fcntl.h>  // for open()
#include "ftp_operations.h"
#include "network_socket.h"

// Helper function to read and parse the FTP response code
static int readResponseCode(int fdSocket) {
    char response[1024];

    // Read the response from the socket
    if (readSocketResponse(fdSocket, response) < 0) {
        return -1; // Return -1 if there's an error reading the response
    }

    // Extract the response code from the server's message
    char statusCode[4];
    strncpy(statusCode, response, 3);
    statusCode[3] = '\0'; // Null-terminate the extracted status code

    return atoi(statusCode); // Convert the status code to an integer and return it
}

int ftpStartConnection(int* fdSocket, const urlInfo* url) {
    if ((*fdSocket = openSocket(url->ipAddress, url->port)) < 0) {
        perror("Error opening socket");
        return -1;
    }

    if (readResponseCode(*fdSocket) != 220) {
        close(*fdSocket); // Close the socket if the response is not 220
        return -1;
    }

    return 0;
}

int ftpLoginIn(const urlInfo* url, int fdSocket) {
    char userCommand[512], passwordCommand[512];

    sprintf(userCommand, "USER %s\r\n", url->user);
    sprintf(passwordCommand, "PASS %s\r\n", url->password);

    if (writeCommandToSocket(fdSocket, userCommand) < 0 ||
        readResponseCode(fdSocket) != 331 ||
        writeCommandToSocket(fdSocket, passwordCommand) < 0 ||
        readResponseCode(fdSocket) != 230) {
        return -1;
    }

    return 0;
}

int ftpPassiveMode(const urlInfo* url, int fdSocket, int* dataSocket) {
    char response[1024];
    int firstIpElement, secondIpElement, thirdIpElement, fourthIpElement;
    int firstPortElement, secondPortElement;

    if (writeCommandToSocket(fdSocket, "PASV\r\n") < 0 ||
        readSocketResponse(fdSocket, response) < 0 ||
        sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
               &firstIpElement, &secondIpElement, &thirdIpElement, &fourthIpElement,
               &firstPortElement, &secondPortElement) != 6) {
        return -1;
    }

    char serverIp[256];
    int serverPort = firstPortElement * 256 + secondPortElement;
    sprintf(serverIp, "%d.%d.%d.%d", firstIpElement, secondIpElement, thirdIpElement, fourthIpElement);

    if ((*dataSocket = openSocket(serverIp, serverPort)) < 0) {
        return -1;
    }

    return 0;
}

int ftpRetrieveFile(const urlInfo* url, int fdSocket, int* fileSize) {
    char retrieveCommand[1024];
    sprintf(retrieveCommand, "RETR %s\r\n", url->urlPath);

    if (writeCommandToSocket(fdSocket, retrieveCommand) < 0 ||
        readResponseCode(fdSocket) != 150) {
        return -1;
    }

    // Additional logic to handle fileSize extraction from the response would go here

    return 0;
}

int ftpDownloadAndCreateFile(const urlInfo* url, int fdDataSocket, int fileSize) {
    int fd = open(url->fileName, O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
        perror("Failed to open file");
        return -1;
    }

    int bytesRead;
    char buffer[1024];
    double totalSize = 0.0;

    while ((bytesRead = read(fdDataSocket, buffer, sizeof(buffer))) > 0) {
        if (write(fd, buffer, bytesRead) < 0) {
            perror("Failed to write to file");
            close(fd);
            return -1;
        }
        totalSize += bytesRead;
        printf("\rDownloading... %d%%", (int)(totalSize / fileSize * 100));
        fflush(stdout);
    }

    printf("\n");
    close(fd);

    if (totalSize != fileSize) {
        fprintf(stderr, "File size mismatch.\n");
        return -1;
    }

    return 0;
}


