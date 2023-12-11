#include "clientFTP.h"

#define RESPONSE_BUFFER_SIZE 1024
#define FTP_READY_CODE 220
#define COMMAND_BUFFER_SIZE 512
#define RESPONSE_BUFFER_SIZE 1024
#define FTP_LOGIN_SUCCESS_CODE 230
#define FTP_NEED_PASSWORD_CODE 331
#define FTP_LOGIN_FAILURE_CODE 530
#define RESPONSE_BUFFER_SIZE 1024
#define FTP_PASSIVE_MODE_CODE 227
#define BUFFER_SIZE 1024 
#define PROGRESS_BAR_WIDTH 20

int establishConnection(int* fdSocket, urlInfo* url) {
    char response[RESPONSE_BUFFER_SIZE] = {0};
    char responseCodeStr[4] = {0};
    int responseCode;

    if ((*fdSocket = createSocketConnection(url->ipAddress, url->port)) < 0) {
        perror("Socket opening error");
        return -1;
    }

    if (socketOutput(*fdSocket, response) < 0) {
        perror("Socket response read error");
        close(*fdSocket);
        return -1;
    }

    strncpy(responseCodeStr, response, 3);
    responseCode = atoi(responseCodeStr);
    if (responseCode != FTP_READY_CODE) {
        perror("Invalid FTP response received");
        close(*fdSocket);
        return -1;
    }

    return 0;
}

int loginToServer(urlInfo* url, int fdSocket) {
    char userCommand[COMMAND_BUFFER_SIZE] = {0};
    char passwordCommand[COMMAND_BUFFER_SIZE] = {0};
    char response[RESPONSE_BUFFER_SIZE] = {0};
    int responseCode;

    sprintf(userCommand, "user %s\n", url->user);
    if (transmitCommand(fdSocket, userCommand) < 0) {
        perror("User command transmission error");
        return -1;
    }

    if (socketOutput(fdSocket, response) < 0) {
        perror("Response read error for user command");
        return -1;
    }

    responseCode = atoi(response);
    if (responseCode != FTP_LOGIN_SUCCESS_CODE && responseCode != FTP_NEED_PASSWORD_CODE) {
        perror("User authentication failure");
        return -1;
    }

    if (responseCode == FTP_NEED_PASSWORD_CODE) {
        sprintf(passwordCommand, "pass %s\n", url->password);
        if (transmitCommand(fdSocket, passwordCommand) < 0) {
            perror("Password command transmission error");
            return -1;
        }

        if (socketOutput(fdSocket, response) < 0) {
            perror("Response read error for password command");
            return -1;
        }

        responseCode = atoi(response);
        if (responseCode == FTP_LOGIN_FAILURE_CODE) {
            perror("Password authentication failure");
            return -1;
        }
    }

    return 0;
}

int switchToPassiveMode(urlInfo* url, int fdSocket, int* dataSocket) {
    char response[RESPONSE_BUFFER_SIZE] = {0};
    int ipParts[4];
    int portParts[2];
    int responseCode;

    if (transmitCommand(fdSocket, "pasv\n") < 0) {
        perror("PASV command error");
        return -1;
    }

    if (socketOutput(fdSocket, response) < 0) {
        perror("PASV response read error");
        return -1;
    }

    sscanf(response, "%d", &responseCode);
    if (responseCode != FTP_PASSIVE_MODE_CODE) {
        perror("Passive mode response invalid");
        return -1;
    }

    if (sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", 
        &ipParts[0], &ipParts[1], &ipParts[2], &ipParts[3], &portParts[0], &portParts[1]) < 6) {
        perror("IP and port parse error from PASV response");
        return -1;
    }

    char serverIp[RESPONSE_BUFFER_SIZE];
    sprintf(serverIp, "%d.%d.%d.%d", ipParts[0], ipParts[1], ipParts[2], ipParts[3]);

    int serverPort = 256 * portParts[0] + portParts[1];

    if ((*dataSocket = createSocketConnection(serverIp, serverPort)) < 0) {
        perror("Data socket opening error");
        return -1;
    }

    return 0;
}

int pullServerFile(urlInfo* url, int fdSocket, int * fileSize){
    char retrieveCommand[1024] = {0};
    char responseToRetrieve[1024] = {0};
    char aux[1024] = {0};
    int codeRetrieve;

    sprintf(retrieveCommand,"retr ./%s\n",url->urlPath);

    if(transmitCommand(fdSocket,retrieveCommand) < 0){
        perror("File retrieval command error");
        return -1;
    }

    if(socketOutput(fdSocket,responseToRetrieve) < 0){
        perror("Response read error for file retrieval");
        return -1;
    }

    memcpy(aux,responseToRetrieve,3);
    codeRetrieve = atoi(aux);

    if(codeRetrieve != 150){
        perror("File retrieval initiation error");
        return -1;
    }
    
    char auxSize[1024];
    memcpy(auxSize,&responseToRetrieve[48],(strlen(responseToRetrieve)-48+1));


    char path[1024];
    sscanf(auxSize,"%s (%d bytes)",path,fileSize);

    return 0;
}

int saveLocally(urlInfo* url, int fdDataSocket, int fileSize){
    int fd;
    int bytesRead;
    double totalSize = 0.0;
    char buffer[BUFFER_SIZE];

    if ((fd = open(url->fileName, O_WRONLY | O_CREAT, 0666)) < 0) {
        perror("File open error for writing");
        return -1;
    }

    printf("\n");

    while ((bytesRead = read(fdDataSocket, buffer, BUFFER_SIZE)) > 0) {
        if (write(fd, buffer, bytesRead) < 0) {
            perror("File write error");
            close(fd);
            return -1;
        }

        totalSize += bytesRead;

        double percentage = totalSize / fileSize;
        int val = (int)(percentage * 100);
        int lpad = (int)(percentage * PROGRESS_BAR_WIDTH);
        int rpad = PROGRESS_BAR_WIDTH - lpad;
        printf("\r%3d%% [%.*s%*s]", val, lpad, "Progress", rpad, "");
        fflush(stdout);
    }

    if (bytesRead < 0) {
        perror("File read error");
        close(fd);
        return -1;
    }

    printf("\n");

    if (totalSize != fileSize) {
        perror("File size inconsistency error");
        close(fd);
        return -1;
    }

    close(fd);
    close(fdDataSocket);

    return 0;
}
