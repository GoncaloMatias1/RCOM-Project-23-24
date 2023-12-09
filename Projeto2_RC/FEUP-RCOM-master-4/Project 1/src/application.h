#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "ll.h"

typedef struct{
    int fdFile;
    int fdPort;
    int fileSize;
    char* fileName; 
    int packetSize;

} ApplicationLayer;

typedef struct 
{
    char* fileName;
    int fileSize;
    
} ControlPacketInformation;

void applicationSetUp(char * fileName, int packetSize, int fdPort);

int readFileInformation(char* fileName);

int sendControlPacket(unsigned char controlByte);

int sendDataPacket();

int sendFile();

int readStartControlPacket(unsigned char * packet);

int processDataPackets(unsigned char* packet);

void readEndControlPacket(unsigned char* packet);

int receiveFile();

