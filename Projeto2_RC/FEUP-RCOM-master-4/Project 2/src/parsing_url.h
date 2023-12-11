#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h>

#define h_addr h_addr_list[0]

/**
 * @struct urlInfo
 * @brief Structure to store URL and related information.
 * 
 * @var urlInfo::user
 * User name for FTP login.
 * @var urlInfo::password
 * Password for FTP login.
 * @var urlInfo::host
 * Hostname or IP address of the FTP server.
 * @var urlInfo::urlPath
 * Path of the file on the FTP server.
 * @var urlInfo::ipAddress
 * IP address of the FTP server.
 * @var urlInfo::fileName
 * Name of the file to be downloaded.
 * @var urlInfo::port
 * Port number of the FTP server.
 */
typedef struct {
    char user[256];
    char password[256];
    char host[256];
    char urlPath[256];
    char ipAddress[256];
    char fileName[256];
    int port;
} urlInfo;

/**
 * @brief Configures the urlInfo structure with default values.
 * 
 * @param url Pointer to the urlInfo structure.
 */
void urlConfig(urlInfo* url);

/**
 * @brief Extracts text from a string up to a specified character.
 * 
 * @param str The string to be processed.
 * @param chr The character up to which text is to be extracted.
 * @return char* A new string containing the extracted text.
 */
char* extractTextBefore(char* str, char chr);

/**
 * @brief Retrieves the IP address of the host and stores it in the urlInfo structure.
 * 
 * @param url Pointer to the urlInfo structure.
 * @return int Returns 0 on success, -1 on failure.
 */
int getHostIP(urlInfo* url);

/**
 * @brief Parses the given URL and fills the urlInfo structure with extracted data.
 * 
 * @param url Pointer to the urlInfo structure to be filled.
 * @param urlGiven The URL to be parsed.
 * @return int Returns 0 on successful parsing, -1 on failure.
 */
int urlParsing(urlInfo* url, char * urlGiven);
