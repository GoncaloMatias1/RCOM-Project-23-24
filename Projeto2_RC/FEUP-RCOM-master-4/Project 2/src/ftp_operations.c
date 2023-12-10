#include "ftp_operations.h"

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


int ftpStartConnection(int* fdSocket, urlInfo* url) {
    char response[RESPONSE_BUFFER_SIZE] = {0};
    char responseCodeStr[4] = {0}; // For storing the first three characters of the response
    int responseCode;

    // Open socket
    if ((*fdSocket = openSocket(url->ipAddress, url->port)) < 0) {
        perror("Error opening socket");
        return -1;
    }

    // Read response from socket
    if (readSocketResponse(*fdSocket, response) < 0) {
        perror("Error reading socket response");
        close(*fdSocket); // Close socket on error
        return -1;
    }

    // Extract and check response code
    strncpy(responseCodeStr, response, 3);
    responseCode = atoi(responseCodeStr);
    if (responseCode != FTP_READY_CODE) {
        perror("Unexpected FTP response code");
        close(*fdSocket); // Close socket on error
        return -1;
    }

    return 0;
}

int ftpLoginIn(urlInfo* url, int fdSocket) {
    char userCommand[COMMAND_BUFFER_SIZE] = {0};
    char passwordCommand[COMMAND_BUFFER_SIZE] = {0};
    char response[RESPONSE_BUFFER_SIZE] = {0};
    int responseCode;

    // Send user command
    sprintf(userCommand, "user %s\n", url->user);
    if (writeCommandToSocket(fdSocket, userCommand) < 0) {
        perror("Error sending user command");
        return -1;
    }

    // Read response to user command
    if (readSocketResponse(fdSocket, response) < 0) {
        perror("Error reading response to user command");
        return -1;
    }
    responseCode = atoi(response);
    if (responseCode != FTP_LOGIN_SUCCESS_CODE && responseCode != FTP_NEED_PASSWORD_CODE) {
        perror("Login unsuccessful with user command");
        return -1;
    }

    // If a password is required, send it
    if (responseCode == FTP_NEED_PASSWORD_CODE) {
        sprintf(passwordCommand, "pass %s\n", url->password);
        if (writeCommandToSocket(fdSocket, passwordCommand) < 0) {
            perror("Error sending password command");
            return -1;
        }

        // Read response to password command
        if (readSocketResponse(fdSocket, response) < 0) {
            perror("Error reading response to password command");
            return -1;
        }
        responseCode = atoi(response);
        if (responseCode == FTP_LOGIN_FAILURE_CODE) {
            perror("Login unsuccessful with password command");
            return -1;
        }
    }

    return 0;
}

int ftpPassiveMode(urlInfo* url, int fdSocket, int* dataSocket) {
    char response[RESPONSE_BUFFER_SIZE] = {0};
    int ipParts[4]; // Elements of the server's IP address
    int portParts[2]; // Elements of the server's port number
    int responseCode;

    // Request passive mode
    if (writeCommandToSocket(fdSocket, "pasv\n") < 0) {
        perror("Error sending PASV command");
        return -1;
    }

    // Read server's response
    if (readSocketResponse(fdSocket, response) < 0) {
        perror("Error reading response to PASV command");
        return -1;
    }

    // Check response code
    sscanf(response, "%d", &responseCode);
    if (responseCode != FTP_PASSIVE_MODE_CODE) {
        perror("Error entering passive mode, unexpected response code");
        return -1;
    }

    // Parse IP address and port number
    if (sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", 
        &ipParts[0], &ipParts[1], &ipParts[2], &ipParts[3], &portParts[0], &portParts[1]) < 6) {
        perror("Error parsing IP address and port from response");
        return -1;
    }

    char serverIp[RESPONSE_BUFFER_SIZE];
    sprintf(serverIp, "%d.%d.%d.%d", ipParts[0], ipParts[1], ipParts[2], ipParts[3]);

    int serverPort = 256 * portParts[0] + portParts[1];

    // Open data socket
    if ((*dataSocket = openSocket(serverIp, serverPort)) < 0) {
        perror("Error opening data socket");
        return -1;
    }

    return 0;
}

int ftpRetrieveFile(urlInfo* url, int fdSocket, int * fileSize){

    char retrieveCommand[1024] = {0};
    char responseToRetrieve[1024] = {0};
    char aux[1024] = {0};
    int codeRetrieve;

    sprintf(retrieveCommand,"retr ./%s\n",url->urlPath);

    if(writeCommandToSocket(fdSocket,retrieveCommand) < 0){
        return -1;
    }


    if(readSocketResponse(fdSocket,responseToRetrieve) < 0){
        return -1;
    }

    memcpy(aux,responseToRetrieve,3);
    codeRetrieve = atoi(aux);

    if(codeRetrieve != 150){
        return -1;
    }
    
    char auxSize[1024];
    memcpy(auxSize,&responseToRetrieve[48],(strlen(responseToRetrieve)-48+1));


    char path[1024];
    sscanf(auxSize,"%s (%d bytes)",path,fileSize);


    return 0;
}

int ftpDownloadAndCreateFile(urlInfo* url, int fdDataSocket, int fileSize){
    int fd;
    int bytesRead;
    double totalSize = 0.0;
    char buffer[BUFFER_SIZE];

    // Open the file for writing
    if ((fd = open(url->fileName, O_WRONLY | O_CREAT, 0666)) < 0) {
        perror("open()");
        return -1;
    }

    printf("\n");

    // Read from data socket and write to file
    while ((bytesRead = read(fdDataSocket, buffer, BUFFER_SIZE)) > 0) {
        if (write(fd, buffer, bytesRead) < 0) {
            perror("write()");
            close(fd); // Close file descriptor on error
            return -1;
        }

        totalSize += bytesRead;

        // Update progress bar
        double percentage = totalSize / fileSize;
        int val = (int)(percentage * 100);
        int lpad = (int)(percentage * PROGRESS_BAR_WIDTH);
        int rpad = PROGRESS_BAR_WIDTH - lpad;
        printf("\r%3d%% [%.*s%*s]", val, lpad, "Sucess", rpad, "");
        fflush(stdout);
    }

    if (bytesRead < 0) {
        perror("read()");
        close(fd); // Close file descriptor on error
        return -1;
    }

    printf("\n");

    // Check if file size matches the expected size
    if (totalSize != fileSize) {
        perror("File size mismatch");
        close(fd); // Close file descriptor
        return -1;
    }

    close(fd);
    close(fdDataSocket);

    return 0;
}
