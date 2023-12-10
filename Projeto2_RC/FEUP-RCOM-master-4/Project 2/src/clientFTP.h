#include "socket_utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int establishConnection(int* fdSocket, urlInfo* url);

int loginToServer(urlInfo* url, int fdSocket);

int passiveMode(urlInfo* url, int fdSocket, int* dataSocket);

int retrieveFile(urlInfo* url, int fdSocket, int * fileSize);

int download_CreateFile(urlInfo* url, int fdDataSocket, int fileSize);
