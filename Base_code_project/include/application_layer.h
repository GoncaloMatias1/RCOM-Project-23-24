// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_
#include "link_layer.h"
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

unsigned char *generateControlPacket(const unsigned int type, const char *dataFileName, long int dataSize);

unsigned char *generateDataPacket(unsigned char sequence, unsigned char *data, int dataSize, int *packetSize);

long int getFileSize(FILE *file);

unsigned char *getData(FILE *fd, long int length);

unsigned char *auxControlPacket(unsigned char *packet, int size, unsigned long int *fileSize);

void auxDataPacket(const unsigned char *packet, const unsigned int packetSize, unsigned char *buf);

#endif // _APPLICATION_LAYER_H_
