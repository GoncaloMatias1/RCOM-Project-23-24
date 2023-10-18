// Application layer protocol implementation

#include <stdio.h>
#include <math.h>
#include "application_layer.h"


void applicationLayer(const char *portName, const char *mode, int baudRate,
                            int maxRetries, int customTimeout, const char *dataFileName)
{
    LinkLayer customLinkLayer;
    strcpy(customLinkLayer.serialPort, portName);
    customLinkLayer.role = (strcmp(mode, "tx") == 0) ? LlTx : LlRx;
    customLinkLayer.baudRate = baudRate;
    customLinkLayer.nRetransmissions = maxRetries;
    customLinkLayer.timeout = customTimeout;

    int fd = llopen(customLinkLayer);
    if (fd < 0) {
        perror("Connection error\n");
        exit(-1);
    }

    switch (customLinkLayer.role) {
    case LlTx: {
        FILE *dataFile = fopen(dataFileName, "rb");
        if (dataFile == NULL) {
            perror("File not found\n");
            exit(-1);
        }

        long int dataSize = getFileSize(dataFile);
        unsigned char *startControlPacket = generateControlPacket(2, dataFileName, dataSize);
        if (llwrite(fd, startControlPacket, startControlPacket[1] + 4) == -1) {
            printf("Exit: error in start packet\n");
            exit(-1);
        }

        unsigned char sequence = 0;
        unsigned char *fileData = getData(dataFile, dataSize);
        long int remainingBytes = dataSize;

        while (remainingBytes > 0) {
            int packetDataSize = (remainingBytes > MAX_PAYLOAD_SIZE) ? MAX_PAYLOAD_SIZE : remainingBytes;
            unsigned char *packetData = (unsigned char *)malloc(packetDataSize);
            memcpy(packetData, fileData, packetDataSize);
            int packetSize;
            unsigned char *packet = generateDataPacket(sequence, packetData, packetDataSize, &packetSize);

            if (llwrite(fd, packet, packetSize) == -1) {
                printf("Exit: error in data packets\n");
                exit(-1);
            }

            remainingBytes -= (long int)MAX_PAYLOAD_SIZE;
            fileData += packetDataSize;
            sequence = (sequence + 1) % 256;
        }

        unsigned char *endControlPacket = generateControlPacket(3, dataFileName, dataSize);
        if (llwrite(fd, endControlPacket, endControlPacket[1] + 4) == -1) {
            printf("Exit: error in end packet\n");
            exit(-1);
        }
        llclose(fd);
        break;
    }
    case LlRx: {
        unsigned char *packet = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
        int packetSize = -1;
        while ((packetSize = llread(fd, packet)) < 0);
        unsigned long int receivedDataSize = 0;
        unsigned char *name = auxControlPacket(packet, packetSize, &receivedDataSize);

        FILE *receivedFile = fopen((char *)name, "wb+");
        while (1) {
            while ((packetSize = llread(fd, packet)) < 0);
            if (packetSize == 0)
                break;
            else if (packet[0] != 3) {
                unsigned char *dataBuffer = (unsigned char *)malloc(packetSize);
                auxDataPacket(packet, packetSize, dataBuffer);
                fwrite(dataBuffer, sizeof(unsigned char), packetSize - 4, receivedFile);
                free(dataBuffer);
            } else
                continue;
        }
        fclose(receivedFile);
        break;
    }
    default:
        exit(-1);
        break;
    }
}

unsigned char *generateControlPacket(const unsigned int type, const char *dataFileName, long int dataSize)
{
    const int L1 = (int)ceil(log2f((float)dataSize) / 8.0);
    const int L2 = strlen(dataFileName);
    unsigned int size = 1 + 2 + L1 + 2 + L2;
    unsigned char *packet = (unsigned char *)malloc(size);

    unsigned int pos = 0;
    packet[pos++] = type;
    packet[pos++] = 0; // T1
    packet[pos++] = L1;

    for (unsigned char i = 0; i < L1; i++) {
        packet[2 + L1 - i] = dataSize & 0xFF;
        dataSize >>= 8;
    }
    pos += L1;
    packet[pos++] = 1; // T2
    packet[pos++] = L2;
    memcpy(packet + pos, dataFileName, L2);
    return packet;
}

unsigned char *generateDataPacket(unsigned char sequence, unsigned char *data, int dataSize, int *packetSize)
{
    *packetSize = 1 + 1 + 2 + dataSize;
    unsigned char *packet = (unsigned char *)malloc(*packetSize);

    packet[0] = 1;
    packet[1] = sequence;
    packet[2] = dataSize >> 8;
    packet[3] = dataSize & 0xFF;
    memcpy(packet + 4, data, dataSize);
    return packet;
}

long int getFileSize(FILE *file) {
    fseek(file, 0, SEEK_END);
    long int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

unsigned char *getData(FILE *fd, long int length)
{
    unsigned char *content = (unsigned char *)malloc(sizeof(unsigned char) * length);
    fread(content, sizeof(unsigned char), length, fd);
    return content;
}

unsigned char *auxControlPacket(unsigned char *packet, int size, unsigned long int *fileSize)
{
    // File Size
    unsigned char bytesFileSize = packet[2];
    unsigned char AuxFileSize[bytesFileSize];
    memcpy(AuxFileSize, packet + 3, bytesFileSize);
    *fileSize = 0;

    for (unsigned int i = 0; i < bytesFileSize; i++)
        *fileSize |= (AuxFileSize[bytesFileSize - i - 1] << (8 * i));

    // File Name
    unsigned char bytesFileName = packet[3 + bytesFileSize + 1];
    unsigned char *name = (unsigned char *)malloc(bytesFileName);
    memcpy(name, packet + 3 + bytesFileSize + 2, bytesFileName);
    return name;
}

void auxDataPacket(const unsigned char *packet, const unsigned int packetSize, unsigned char *buf)
{
    memcpy(buf, packet + 4, packetSize - 4);
    buf += packetSize - 4;
}
