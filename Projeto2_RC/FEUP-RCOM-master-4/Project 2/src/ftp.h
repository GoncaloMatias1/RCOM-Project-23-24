#include "socket.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int ftpStartConnection(int* fdSocket, urlInfo* url);

int ftpLoginIn(urlInfo* url, int fdSocket);

int ftpPassiveMode(urlInfo* url, int fdSocket, int* dataSocket);

int ftpRetrieveFile(urlInfo* url, int fdSocket, int * fileSize);

int ftpDownloadAndCreateFile(urlInfo* url, int fdDataSocket, int fileSize);
