#include "utils.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>


//ConnectionInfo struct setup
speed_t checkBaudrate(char * baudRate);
void infoSetup(char * baudRate);

//Auxiliary functions
int verifyControlByte(unsigned char byte);
void responseStateMachine(enum state* currentState, unsigned char byte, unsigned char* controlByte);
void informationFrameStateMachine(enum state* currentState, unsigned char byte, unsigned char* controlByte);
int verifyFrame(unsigned char* frame,int length);
int processControlByte(int fd, unsigned char *controlByte);

//Transmitter
void readReceiverResponse(int fd);

//Receiver
void readTransmitterResponse(int fd);
int readTransmitterFrame(int fd, unsigned char * buffer);

// ll functions
int llopen(char* port, int flag, char* baudrate);
int llwrite(int fd, unsigned char* packet, int length);
int llread(int fd, unsigned char* buf);
int llclose(int fd, int flag);

void sigAlarmHandler();

void initializeAlarm();

void disableAlarm();


typedef struct {
    //alarm info
    int numTries;
    int alarmFlag;
    int ns;
    speed_t baudRate;
    int firstTime;

}   ConnectionInfo;
