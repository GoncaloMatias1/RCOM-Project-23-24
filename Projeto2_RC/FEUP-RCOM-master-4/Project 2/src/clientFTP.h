#include "socket_utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int establishConnection(int* fdSocket, urlInfo* url);

int loginToServer(urlInfo* url, int fdSocket);

int switchToPassiveMode(urlInfo* url, int fdSocket, int* dataSocket);

int pullServerFile(urlInfo* url, int fdSocket, int * fileSize);

int saveLocally(urlInfo* url, int fdDataSocket, int fileSize);
