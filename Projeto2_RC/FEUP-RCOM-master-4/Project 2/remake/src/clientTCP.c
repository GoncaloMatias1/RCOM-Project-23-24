//#include "include/clientTCP.h"
#include "clientTCP.h"
int ftpStartConnection(int* fdSocket, urlInfo* url) {
    char response[1024] = {0};
    int code;

    if ((*fdSocket = openSocket(url->ipAddress, url->port)) < 0) {
        perror("Error opening socket!\n");
        return -1;
    }

    readSocketResponse(*fdSocket, response);
    code = atoi(response);

    if (code != 220) {
        return -1;
    }

    return 0;
}

int ftpLoginIn(urlInfo* url, int fdSocket) {
    char userCommand[512] = {0};
    char passwordCommand[512] = {0};
    char responseToUserCommand[1024] = {0};
    char responseToPasswordCommand[1024] = {0};
    int codeUserCommand, codePasswordCommand;

    sprintf(userCommand, "user %s\n", url->user);
    sprintf(passwordCommand, "pass %s\n", url->password);

    if (writeCommandToSocket(fdSocket, userCommand) < 0 || readSocketResponse(fdSocket, responseToUserCommand) < 0) {
        return -1;
    }

    sscanf(responseToUserCommand, "%d", &codeUserCommand);

    if (codeUserCommand != 230 && codeUserCommand != 331) {
        return -1;
    }

    if (writeCommandToSocket(fdSocket, passwordCommand) < 0 || readSocketResponse(fdSocket, responseToPasswordCommand) < 0) {
        return -1;
    }

    sscanf(responseToPasswordCommand, "%d", &codePasswordCommand);

    if (codePasswordCommand == 530) {
        return -1;
    }

    return 0;
}

int ftpPassiveMode(urlInfo* url, int fdSocket, int* dataSocket) {
    char responseToPassiveMode[1024] = {0};

    if (writeCommandToSocket(fdSocket, "pasv\n") < 0 || readSocketResponse(fdSocket, responseToPassiveMode) < 0) {
        return -1;
    }

    int firstIp, secondIp, thirdIp, fourthIp, firstPort, secondPort;
    int codePassiveMode;

    sscanf(responseToPassiveMode, "%d Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &codePassiveMode, &firstIp, &secondIp, &thirdIp, &fourthIp, &firstPort, &secondPort);

    char serverIp[1024];
    sprintf(serverIp, "%d.%d.%d.%d", firstIp, secondIp, thirdIp, fourthIp);

    int serverPort = 256 * firstPort + secondPort;

    if ((*dataSocket = openSocket(serverIp, serverPort)) < 0) {
        return -1;
    }

    return 0;
}

int ftpRetrieveFile(urlInfo* url, int fdSocket, int * fileSize) {
    char retrieveCommand[1024] = {0};
    char responseToRetrieve[1024] = {0};

    sprintf(retrieveCommand, "retr ./%s\n", url->urlPath);

    if (writeCommandToSocket(fdSocket, retrieveCommand) < 0 || readSocketResponse(fdSocket, responseToRetrieve) < 0) {
        return -1;
    }

    sscanf(responseToRetrieve, "150 Opening BINARY mode data connection for %s (%d bytes)", url->fileName, fileSize);

    return 0;
}

int ftpDownloadAndCreateFile(urlInfo* url, int fdDataSocket, int fileSize) {
    int fd, bytesRead;
    double totalSize = 0.0;
    char buffer[1024];

    if ((fd = open(url->fileName, O_WRONLY | O_CREAT, 0666)) < 0) {
        perror("open()\n");
        return -1;
    }

    while ((bytesRead = read(fdDataSocket, buffer, sizeof(buffer)))) {
        if (bytesRead < 0 || write(fd, buffer, bytesRead) < 0) {
            perror("read()/write()\n");
            return -1;
        }

        totalSize += bytesRead;
        double percentage = totalSize / fileSize;
        printf("\r%3d%% [%.*s%*s]", (int)(percentage * 100), (int)(percentage * 60), "############################################################", 60 - (int)(percentage * 60), "");
        fflush(stdout);
    }

    printf("\n");

    if (totalSize != fileSize) {
        return -1;
    }

    close(fd);
    close(fdDataSocket);

    return 0;
}

int openSocket(char* ipAddress, int port){
    int	sockfd;
	struct	sockaddr_in server_addr;

    bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ipAddress);	
	server_addr.sin_port = htons(port);		

    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("socket()");
        return -1;
    }
	/*connect to the server*/
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("connect()");
        return -2;
	}

	return sockfd;
}

int writeCommandToSocket(int fdSocket,char* command){

    int commandSize = strlen(command);
    int bytesWritten;

    if((bytesWritten = write(fdSocket,command,commandSize)) < commandSize){
        perror("Error writing command to socket\n");
        return -1;
    }

    return 0;
}

int readSocketResponse(int fdSocket,char * response){
    FILE* file = fdopen(fdSocket,"r");

    do {
		memset(response, 0, 1024);
		response = fgets(response, 1024, file);
		printf("%s", response);

	} while (!('1' <= response[0] && response[0] <= '5') || response[3] != ' ');


    return 0;
}
