#include "socket_utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int ftpStartConnection(int* fdSocket, urlInfo* url);

int loginToServer(urlInfo* url, int fdSocket);

int passive_Mode(urlInfo* url, int fdSocket, int* dataSocket);

int retrieveFile(urlInfo* url, int fdSocket, int * fileSize);

int ftpDownloadAndCreateFile(urlInfo* url, int fdDataSocket, int fileSize);
