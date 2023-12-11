#include "socket_utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * @brief Establishes a connection to an FTP server.
 * 
 * @param fdSocket Pointer to the socket file descriptor.
 * @param url Pointer to a urlInfo structure containing URL details.
 * @return int Returns 0 on success, -1 on failure.
 */
int establishConnection(int* fdSocket, urlInfo* url);

/**
 * @brief Logs into the FTP server with the credentials provided in the urlInfo.
 * 
 * @param url Pointer to a urlInfo structure containing login credentials.
 * @param fdSocket Socket file descriptor for the connection.
 * @return int Returns 0 on successful login, -1 on failure.
 */
int loginToServer(urlInfo* url, int fdSocket);

/**
 * @brief Switches the FTP connection to passive mode.
 * 
 * @param url Pointer to a urlInfo structure.
 * @param fdSocket Socket file descriptor for the connection.
 * @param dataSocket Pointer to the data socket file descriptor.
 * @return int Returns 0 on success, -1 on failure.
 */
int switchToPassiveMode(urlInfo* url, int fdSocket, int* dataSocket);

/**
 * @brief Initiates the process to retrieve a file from the FTP server.
 * 
 * @param url Pointer to a urlInfo structure with the file's URL.
 * @param fdSocket Socket file descriptor for the connection.
 * @param fileSize Pointer to store the size of the file being downloaded.
 * @return int Returns 0 on success, -1 on failure.
 */
int pullServerFile(urlInfo* url, int fdSocket, int * fileSize);

/**
 * @brief Saves the retrieved file locally.
 * 
 * @param url Pointer to a urlInfo structure containing file information.
 * @param fdDataSocket Data socket file descriptor.
 * @param fileSize Size of the file to be saved.
 * @return int Returns 0 on success, -1 on failure.
 */
int saveLocally(urlInfo* url, int fdDataSocket, int fileSize);
