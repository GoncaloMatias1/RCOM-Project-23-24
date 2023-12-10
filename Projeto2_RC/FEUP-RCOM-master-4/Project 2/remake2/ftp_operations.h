#ifndef FTP_OPERATIONS_H
#define FTP_OPERATIONS_H

#include "url_parser.h" // This includes all necessary type definitions

int ftpStartConnection(int* fdSocket, const urlInfo* url);
int ftpLoginIn(const urlInfo* url, int fdSocket);
int ftpPassiveMode(const urlInfo* url, int fdSocket, int* dataSocket);
int ftpRetrieveFile(const urlInfo* url, int fdSocket, int* fileSize);
int ftpDownloadAndCreateFile(const urlInfo* url, int fdDataSocket, int fileSize);

#endif // FTP_OPERATIONS_H
