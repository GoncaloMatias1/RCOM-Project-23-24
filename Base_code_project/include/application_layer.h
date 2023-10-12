// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include "link_layer.h"
#include <stdio.h>

// Application layer main function.
// Arguments:
//   portName: Serial port name (e.g., /dev/ttyS0).
//   mode: Application mode {"tx", "rx"}.
//   baudRate: Baudrate of the serial port.
//   maxRetries: Maximum number of frame retries.
//   customTimeout: Frame timeout.
//   dataFileName: Name of the file to send / receive.
void customApplicationLayer(const char *portName, const char *mode, int baudRate,
                           int maxRetries, int customTimeout, const char *dataFileName);

// Function to parse a control packet.
unsigned char *parseControlPacket(unsigned char *packet, int size, unsigned long int *fileSize);

// Function to parse a data packet.
void parseDataPacket(const unsigned char *packet, const unsigned int packetSize, unsigned char *buffer);

// Function to create a control packet.
unsigned char *generateControlPacket(const unsigned int type, const char *dataFileName, long int dataSize, unsigned int *size);

// Function to create a data packet.
unsigned char *generateDataPacket(unsigned char sequence, unsigned char *data, int dataSize, int *packetSize);

// Function to read data from a file.
unsigned char *getData(FILE *fd, long int fileLength);

#endif // _APPLICATION_LAYER_H_
