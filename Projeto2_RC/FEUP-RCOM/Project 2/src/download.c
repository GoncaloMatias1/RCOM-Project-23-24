#include "clientFTP.h"

void handleError(const char* errorMessage, int socket) {
    perror(errorMessage);
    if (socket >= 0) {
        close(socket);
    }
    exit(-1);
}

int main(int argc, char** argv){
    if(argc != 2){handleError("Incorrect argument count!\n", -1);}

    urlInfo url;
    int fdSocket = -1, fdDataSocket = -1, fileSize;

    urlConfig(&url);

    if(urlParsing(&url, argv[1]) < 0){handleError("Failed to parse URL!\n", -1);}

    if(getHostIP(&url) < 0){handleError("Unable to obtain IP Address!\n", -1);}

    if(establishConnection(&fdSocket, &url) < 0){handleError("Connection initialization failed!\n", -1);}

    if(loginToServer(&url, fdSocket) < 0){handleError("Login failure!\n", fdSocket);}

    if(switchToPassiveMode(&url, fdSocket, &fdDataSocket) < 0){handleError("Passive mode entry error!\n", fdSocket);}

    if(pullServerFile(&url, fdSocket, &fileSize) < 0){handleError("File retrieval failed!\n", fdSocket);}

    if(saveLocally(&url, fdDataSocket, fileSize) < 0){handleError("Download or file creation error!\n", fdSocket);}

    close(fdSocket);

    return 0;
}
