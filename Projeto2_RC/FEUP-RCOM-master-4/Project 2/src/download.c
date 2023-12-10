#include "clientFTP.h"

void handleError(const char* errorMessage, int socket) {
    perror(errorMessage);
    if (socket >= 0) {
        close(socket);
    }
    exit(-1);
}

int main(int argc, char** argv){
    if(argc != 2){handleError("Wrong number of arguments!\n", -1);}

    urlInfo url;
    int fdSocket = -1, fdDataSocket = -1, fileSize;

    initializeUrlInfo(&url);

    if(parseUrlInfo(&url, argv[1]) < 0){handleError("Error parsing url!\n", -1);}

    if(getIpAddressFromHost(&url) < 0){handleError("Error getting IP Address!\n", -1);}

    if(establishConnection(&fdSocket, &url) < 0){handleError("Error initializing connection!\n", -1);}

    if(loginToServer(&url, fdSocket) < 0){handleError("Error while logging in!\n", fdSocket);}

    if(switchToPassiveMode(&url, fdSocket, &fdDataSocket) < 0){handleError("Error entering in passive mode!\n", fdSocket);}

    if(pullServerFile(&url, fdSocket, &fileSize) < 0){handleError("Error retrieving file!\n", fdSocket);}

    if(saveLocally(&url, fdDataSocket, fileSize) < 0){handleError("Error downloading/creating file!\n", fdSocket);}

    close(fdSocket);

    return 0;
}
