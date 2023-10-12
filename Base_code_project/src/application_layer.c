// Modified application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void customApplicationLayer(const char *portName, const char *mode, int baudRate,
                            int maxRetries, int customTimeout, const char *dataFileName)
{
    CustomLinkLayer customLinkLayer;
    strcpy(customLinkLayer.portName, portName);
    customLinkLayer.mode = (strcmp(mode, "tx") == 0) ? LlTx : LlRx;
    customLinkLayer.baudRate = baudRate;
    customLinkLayer.maxRetries = maxRetries;
    customLinkLayer.customTimeout = customTimeout;

    int fd = customLLOpen(customLinkLayer);
    if (fd < 0) {
        perror("Connection error\n");
        exit(-1);
    }

    switch (customLinkLayer.mode) {

    case LlTx: {

        FILE *dataFile = fopen(dataFileName, "rb");
        if (dataFile == NULL) {
            perror("File not found\n");
            exit(-1);
        }

        int startPosition = ftell(dataFile);
        fseek(dataFile, 0L, SEEK_END);
        long int dataSize = ftell(dataFile) - startPosition;
        fseek(dataFile, startPosition, SEEK_SET);

        unsigned int cpSize;
        unsigned char *startControlPacket = generateControlPacket(2, dataFileName, dataSize, &cpSize);
        if (customLLWrite(fd, startControlPacket, cpSize) == -1) {
            printf("Exit: error in start packet\n");
            exit(-1);
        }

        unsigned char sequence = 0;
        unsigned char *fileData = getData(dataFile, dataSize);
        long int remainingBytes = dataSize;

        while (remainingBytes >= 0) {

            int packetDataSize = remainingBytes > (long int)MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : remainingBytes;
            unsigned char *packetData = (unsigned char *)malloc(packetDataSize);
            memcpy(packetData, fileData, packetDataSize);
            int packetSize;
            unsigned char *packet = generateDataPacket(sequence, packetData, packetDataSize, &packetSize);

            if (customLLWrite(fd, packet, packetSize) == -1) {
                printf("Exit: error in data packets\n");
                exit(-1);
            }

            remainingBytes -= (long int)MAX_PAYLOAD_SIZE;
            fileData += packetDataSize;
            sequence = (sequence + 1) % 255;
        }

        unsigned char *endControlPacket = generateControlPacket(3, dataFileName, dataSize, &cpSize);
        if (customLLWrite(fd, endControlPacket, cpSize) == -1) {
            printf("Exit: error in end packet\n");
            exit(-1);
        }
        customLLClose(fd);
        break;
    }

    case LlRx: {

        unsigned char *packet = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
        int packetSize = -1;
        while ((packetSize = customLLRead(fd, packet)) < 0);
        unsigned long int receivedDataSize = 0;
        unsigned char *name = parseControlPacket(packet, packetSize, &receivedDataSize);

        FILE *receivedFile = fopen((char *)name, "wb+");
        while (1) {
            while ((packetSize = customLLRead(fd, packet)) < 0);
            if (packetSize == 0)
                break;
            else if (packet[0] != 3) {
                unsigned char *dataBuffer = (unsigned char *)malloc(packetSize);
                parseDataPacket(packet, packetSize, dataBuffer);
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

unsigned char *generateControlPacket(const unsigned int type, const char *dataFileName, long int dataSize, unsigned int *size)
{
    const int L1 = (int)ceil(log2f((float)dataSize) / 8.0);
    const int L2 = strlen(dataFileName);
    *size = 1 + 2 + L1 + 2 + L2;
    unsigned char *packet = (unsigned char *)malloc(*size);

    unsigned int pos = 0;
    packet[pos++] = type;
    packet[pos++] = 0;
    packet[pos++] = L1;

    for (unsigned char i = 0; i < L1; i++)
    {
        packet[2 + L1 - i] = dataSize & 0xFF;
        dataSize >>= 8;
    }
    pos += L1;
    packet[pos++] = 1;
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
    packet[2] = dataSize >> 8 & 0xFF;
    packet[3] = dataSize & 0xFF;
    memcpy(packet + 4, data, dataSize);

    return packet;
}
