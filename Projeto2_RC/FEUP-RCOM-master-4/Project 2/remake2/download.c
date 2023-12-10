#include "ftp_operations.h"
#include <unistd.h>


void printErrorAndExit(const char* message) {
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

int downloadFile(const char* fileUrl) {
    urlInfo url;
    int fdSocket, fdDataSocket, fileSize;

    initializeUrlInfo(&url);

    if (parseUrlInfo(&url, fileUrl) < 0) {
        printErrorAndExit("Error parsing URL.");
    }

    if (getIpAddressFromHost(&url) < 0) {
        printErrorAndExit("Error getting IP Address.");
    }

    if (ftpStartConnection(&fdSocket, &url) < 0) {
        printErrorAndExit("Error initializing connection.");
    }

    if (ftpLoginIn(&url, fdSocket) < 0) {
        printErrorAndExit("Error while logging in.");
    }

    if (ftpPassiveMode(&url, fdSocket, &fdDataSocket) < 0) {
        printErrorAndExit("Error entering in passive mode.");
    }

    if (ftpRetrieveFile(&url, fdSocket, &fileSize) < 0) {
        printErrorAndExit("Error retrieving file.");
    }

    if (ftpDownloadAndCreateFile(&url, fdDataSocket, fileSize) < 0) {
        printErrorAndExit("Error downloading/creating file.");
    }

    close(fdSocket);
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printErrorAndExit("Usage: download <URL>");
    }

    return downloadFile(argv[1]);
}
