#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include "parsing_url.h"

/**
 * @brief Creates a socket and connects to the specified IP address and port.
 * 
 * @param ipAddress IP address to connect to.
 * @param port Port number to connect to.
 * @return int Returns a socket file descriptor on success, -1 on failure to create socket, -2 on failure to connect.
 */
int createSocketConnection(char* ipAddress, int port);

/**
 * @brief Transmits a command to the specified socket.
 * 
 * @param fdSocket File descriptor of the socket to which the command is to be sent.
 * @param command The command string to be sent.
 * @return int Returns 0 on successful transmission, -1 if the command is NULL, -2 on write error, -3 on incomplete write.
 */
int transmitCommand(int fdSocket, const char* command);

/**
 * @brief Reads the output from a socket into a given response buffer.
 * 
 * @param fdSocket File descriptor of the socket to read from.
 * @param response Buffer to store the response.
 * @return int Returns 0 on success, -1 if the response buffer is NULL, -2 on file descriptor open error, -3 on read error.
 */
int socketOutput(int fdSocket, char* response);
