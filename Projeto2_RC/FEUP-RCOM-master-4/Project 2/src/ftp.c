#include "ftp.h"

int ftpStartConnection(int* fdSocket, urlInfo* url){

    char response[1024] = {0};
    int code;
    char aux[1024] = {0};

    if((*fdSocket = openSocket(url->ipAddress,url->port)) < 0){
        perror("Error opening socket!\n");
        return -1;
    }

    readSocketResponse(*fdSocket,response);

    memcpy(aux,response,3);
    code = atoi(aux);

    if(code != 220){
        return -1;
    }

    return 0;

}

int ftpLoginIn(urlInfo* url, int fdSocket){
    char userCommand[512] = {0};
    char passwordCommand[512] = {0};
    char responseToUserCommand[1024] = {0};
    char responseToPasswordCommand[1024] = {0};
    char aux[1024] = {0};
    int codeUserCommand;
    int codePasswordCommand;

    sprintf(userCommand,"user %s\n",url->user);
    sprintf(passwordCommand,"pass %s\n",url->password);


    if(writeCommandToSocket(fdSocket,userCommand) < 0){
        return -1;
    }
    
    if(readSocketResponse(fdSocket,responseToUserCommand) < 0){
        return -1;
    }

    memcpy(aux,responseToUserCommand,3);

    codeUserCommand = atoi(aux);


    if(codeUserCommand != 230 && codeUserCommand != 331){
        return -1;
    }


    if(writeCommandToSocket(fdSocket,passwordCommand) < 0){
        return -1;
    }

    if(readSocketResponse(fdSocket,responseToPasswordCommand) < 0){
        return -1;
    }

    memcpy(aux,responseToPasswordCommand,3);
    codePasswordCommand = atoi(aux);

    if(codePasswordCommand == 530){
        return -1;
    }


    return 0;
}

int ftpPassiveMode(urlInfo* url, int fdSocket, int* dataSocket){
    
    char responseToPassiveMode[1024] = {0};
    
    if(writeCommandToSocket(fdSocket,"pasv\n") < 0){
        return -1;
    }

    if(readSocketResponse(fdSocket,responseToPassiveMode) < 0){
        return -1;
    }


    int firstIpElement, secondIpElement, thirdIpElement, fourthIpElement;
    int firstPortElement, secondPortElement;
    int codePassiveMode;

    char serverIp[1024] = {0};
    int serverPort;
    char aux[1024] = {0};

    memcpy(aux,responseToPassiveMode,3);
    codePassiveMode = atoi(aux);

    if(codePassiveMode != 227){
        return -1;
    }


    sscanf(responseToPassiveMode,"227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",&firstIpElement,&secondIpElement,&thirdIpElement,&fourthIpElement,&firstPortElement,&secondPortElement);

    sprintf(serverIp,"%d.%d.%d.%d",firstIpElement,secondIpElement,thirdIpElement,fourthIpElement);

    serverPort = 256*firstPortElement+secondPortElement;


    if((*dataSocket = openSocket(serverIp,serverPort)) < 0){
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
    char buffer[1024];

    if((fd = open(url->fileName, O_WRONLY | O_CREAT, 0666)) < 0) {
		perror("open()\n");
		return -1;
	}

    printf("\n");

    while ((bytesRead = read(fdDataSocket, buffer, 1024))) {

		if (bytesRead < 0) {
			perror("read()\n");
			return -1;
		}

		if (write(fd, buffer, bytesRead) < 0) {
			perror("write()\n");
			return -1;
		}

        totalSize += bytesRead;

        //progress bar
        double percentage = totalSize/fileSize;
        int val = (int) (percentage * 100);
        int lpad = (int) (percentage * 60);
        int rpad = 60 - lpad;
        printf("\r%3d%% [%.*s%*s]", val, lpad, "############################################################", rpad, "");
        fflush(stdout);
	}

    printf("\n");

    if(totalSize != fileSize){
        return -1;
    }

    close(fd);
    close(fdDataSocket);

    return 0;
}
