#include "application.h"


ApplicationLayer app;

ControlPacketInformation packetInfo;

void applicationSetUp(char * fileName, int packetSize, int fdPort){
    app.fileName = fileName;
    app.packetSize = packetSize;
    app.fdPort = fdPort;
}

int readFileInformation(char* fileName){

    int fd;
    struct stat status;

    if((fd = open(fileName,O_RDONLY)) < 0){
        perror("Error opening file!\n");
        return -1;
    }

    if(stat(fileName,&status) < 0){
        perror("Error reading file information!\n");
        return -1;
    }

    app.fdFile = fd;
    app.fileName = fileName;
    app.fileSize = status.st_size;

    return 0;
}


int sendControlPacket(unsigned char controlByte){
    int packetIndex = 0;
    int numBytesFileSize = sizeof(app.fileSize);

    unsigned char packet[5+numBytesFileSize + strlen(app.fileName)];

    packet[packetIndex] = controlByte;
    packetIndex++;

    packet[packetIndex] = FILE_NAME_FLAG;  //flag nome do ficheiro
    packetIndex++;
    
    packet[packetIndex] = strlen(app.fileName);  // lenght do nome do ficheiro
    packetIndex++;

    for (size_t i = 0; i < strlen(app.fileName); i++)
    {
        packet[packetIndex] = app.fileName[i];
        packetIndex++;
    }

    packet[packetIndex] = FILE_SIZE_FLAG;   //flag que indica o tamanho do ficheiro
    packetIndex++;

    packet[packetIndex] = numBytesFileSize;
    packetIndex++;

    packet[packetIndex] = (app.fileSize >> 24) & BYTE_MASK;
    packetIndex++;

    packet[packetIndex] = (app.fileSize >> 16) & BYTE_MASK;
    packetIndex++;

    packet[packetIndex] = (app.fileSize >> 8) & BYTE_MASK;
    packetIndex++;

    packet[packetIndex] = app.fileSize & BYTE_MASK;
    packetIndex++;
    
    if(llwrite(app.fdPort,packet,packetIndex) < packetIndex){
        printf("Error writing control packet to serial port!\n");
        return -1;
    }
    
    return 0;
}


int sendDataPacket(){

    int numPacketsSent = 0;
    int numPacketsToSend = app.fileSize/app.packetSize;            // numero máximo de de octetos num packet
    unsigned char buffer[app.packetSize];
    int bytesRead = 0;
    int length = 0;
    
    if(app.fileSize%app.packetSize != 0){
        numPacketsToSend++;
    }

    while(numPacketsSent < numPacketsToSend){

        if((bytesRead = read(app.fdFile,buffer,app.packetSize)) < 0){
            printf("Error reading file\n");
        }
        unsigned char packet[4+app.packetSize];
        packet[0] = DATA_FLAG;
        packet[1] = numPacketsSent % 255;
        packet[2] = bytesRead / 256;
        packet[3] = bytesRead % 256;
        memcpy(&packet[4],buffer,bytesRead);
        length = bytesRead + 4;

        if(llwrite(app.fdPort,packet,length) < length){
            printf("Error writing data packet to serial port!\n");
            return -1;
        }
        numPacketsSent++;
    }

    return 0;
}

int sendFile(){

    if(readFileInformation(app.fileName) < 0){
        printf("Error reading file information!\n");
        return -1;
    }
    
    if(sendControlPacket(START_FLAG) < 0){
        printf("Error sending start control packet!\n");
        return -1;
    }
    
    if(sendDataPacket() < 0){
        printf("Error sending data packet!\n");
        return -1;
    }
    
    if(sendControlPacket(END_FLAG) < 0){
        printf("Error sending end control packet!\n");
        return -1;
    }

    return 0;
}


int readStartControlPacket(unsigned char * packet){
    int packetIndex = 1;
    int fileNameLength = 0;
    int fileSize = 0;
    char* fileName;


    if(packet[packetIndex] == FILE_NAME_FLAG){              //flag do nome do ficheiro
        packetIndex++;
        fileName = (char*) malloc(packet[packetIndex]+1);
        fileNameLength = packet[packetIndex];
        packetIndex++;

        for (size_t i = 0; i < fileNameLength; i++)
        {
            fileName[i] = packet[packetIndex];
            packetIndex++;

            if(i == fileNameLength-1){
                fileName[fileNameLength] = '\0';
            }
        }
    }

    if(packet[packetIndex] == FILE_SIZE_FLAG){
        packetIndex+=2;
        fileSize += packet[packetIndex] << 24;
        packetIndex++;
        fileSize += packet[packetIndex] << 16;
        packetIndex++;
        fileSize += packet[packetIndex] << 8;
        packetIndex++;
        fileSize += packet[packetIndex];
        
    }

    packetInfo.fileName = fileName;
    packetInfo.fileSize = fileSize;

    app.fdFile = open(fileName,O_WRONLY | O_CREAT | O_APPEND, 0664);

    return 0;
}

void readEndControlPacket(unsigned char* packet){
    int packetIndex = 1;
    int fileNameLength = 0;
    int fileSize = 0;
    char* fileName;

    if(packet[packetIndex] == FILE_NAME_FLAG){              //flag do nome do ficheiro
        packetIndex++;
        fileName = (char*) malloc(packet[packetIndex]+1);
        fileNameLength = packet[packetIndex];
        packetIndex++;

        for (size_t i = 0; i < fileNameLength; i++)
        {
            fileName[i] = packet[packetIndex];
            packetIndex++;

            if(i == fileNameLength-1){
                fileName[fileNameLength] = '\0';
            }
        }
    }

    if(packet[packetIndex] == FILE_SIZE_FLAG){
        packetIndex+=2;
        fileSize += packet[packetIndex] << 24;
        packetIndex++;
        fileSize += packet[packetIndex] << 16;
        packetIndex++;
        fileSize += packet[packetIndex] << 8;
        packetIndex++;
        fileSize += packet[packetIndex];
        
    }

    if(packetInfo.fileSize != fileSize || strcmp(packetInfo.fileName,fileName)!= 0){
        printf("Start packet and end packet have different file name and/or different file size\n");
    }

}

int processDataPackets(unsigned char* packet){
    
    int informationSize = 256*packet[2]+packet[3];
 
    write(app.fdFile,packet+4,informationSize);
    return 0;
}

int receiveFile(){
    unsigned char buffer[app.packetSize+4];
    int stop = 0;
    int lastSequenceNumber = -1;
    int currentSequenceNumber;


    while(!stop){
        llread(app.fdPort,buffer);
        if(buffer[0] == START_FLAG){
            readStartControlPacket(buffer);
        }
        else if(buffer[0] == DATA_FLAG){
            currentSequenceNumber = (int)(buffer[1]);
            if(lastSequenceNumber >= currentSequenceNumber && lastSequenceNumber != 254)
                continue;
            lastSequenceNumber = currentSequenceNumber;
            processDataPackets(buffer);
        }
        else if(buffer[0] == END_FLAG){
            readEndControlPacket(buffer);
            stop = 1;
        }
    }

    close(app.fdFile);

    return 0;
}
