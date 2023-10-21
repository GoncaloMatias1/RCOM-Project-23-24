#include "application_layer.h"
#include "link_layer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONTROL_DATA    0x01
#define CONTROL_START   0x02
#define CONTROL_END     0x03

#define FILE_SIZE       0x00
#define FILE_NAME       0x01

#define OCTET_MULT      256
#define MAX_PACKET_SIZE 1024

size_t create_control_packet(uint8_t control, const char* filename, size_t file_size, uint8_t* packet) {
    const size_t file_length = strlen(filename);

    if (file_length > 0xff || (file_length + sizeof(size_t) + 1) > MAX_PACKET_SIZE) {
        fprintf(stderr, "Invalid control packet parameters\n");
        return 0;
    }

    packet[0] = control;
    packet[1] = FILE_SIZE;
    packet[2] = sizeof(size_t);
    memcpy(packet + 3, &file_size, sizeof(size_t));
    packet[3 + sizeof(size_t)] = FILE_NAME;
    packet[4 + sizeof(size_t)] = (uint8_t) file_length;
    memcpy(packet + 5 + sizeof(size_t), filename, file_length);

    return 5 + sizeof(size_t) + file_length;
}

int send_control_packet(uint8_t control, const char* filename, size_t file_size) {
    uint8_t packet[MAX_PACKET_SIZE];
    size_t plength = create_control_packet(control, filename, file_size, packet);

    if (plength == 0) {
        return -1;
    }
    return llwrite(packet, plength) != -1 ? 0 : -1;
}

int send_data_packet(const uint8_t* data, size_t length) {
    uint8_t packet[MAX_PACKET_SIZE];
    packet[0] = CONTROL_DATA;
    packet[1] = (uint8_t) (length / OCTET_MULT);
    packet[2] = (uint8_t) (length % OCTET_MULT);
    memcpy(packet + 3, data, length);

    return llwrite(packet, length + 3) != -1 ? 0 : -1;
}

int send(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("Error opening file");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (send_control_packet(CONTROL_START, filename, file_size) == -1) {
        fclose(f);
        return -1;
    }

    uint8_t buf[MAX_PACKET_SIZE - 3];
    size_t bytes_read;
    while ((bytes_read = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (send_data_packet(buf, bytes_read) == -1) {
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return send_control_packet(CONTROL_END, filename, file_size);
}

int read_control_packet(uint8_t control, uint8_t* buf, size_t* file_size, char* received_filename) {
    int size = llread(buf);
    if (size < 0 || buf[0] != control) {
        return -1;
    }

    uint8_t type;
    size_t length;
    size_t offset = 1;

    while (offset < size) {
        type = buf[offset++];
        length = buf[offset++];
        
        if (type == FILE_SIZE && length == sizeof(size_t)) {
            memcpy(file_size, buf + offset, sizeof(size_t));
            offset += sizeof(size_t);
        } else if (type == FILE_NAME && length <= MAX_PACKET_SIZE - offset) {
            memcpy(received_filename, buf + offset, length);
            offset += length;
        } else {
            return -1;
        }
    }

    return 0;
}

int receive(const char* filename) {
    uint8_t buf[MAX_PACKET_SIZE];
    size_t file_size;

    char received_filename[0xff];
    if (read_control_packet(CONTROL_START, buf, &file_size, received_filename) == -1) {
        return -1;
    }

    FILE* fs = fopen(filename, "wb");
    if (!fs) {
        perror("Failed to open file");
        return -1;
    }

    while (llread(buf) > 0) {
        if (buf[0] == CONTROL_END) {
            break;
        } else if (buf[0] != CONTROL_DATA) {
            fclose(fs);
            return -1;
        }

        size_t length = buf[1] * OCTET_MULT + buf[2];
        fwrite(buf + 3, sizeof(uint8_t), length, fs);
    }

    fclose(fs);
    return 0;
}

void applicationLayer(const char *portName, const char *mode, int baudRate,
                            int maxRetries, int customTimeout, const char *dataFileName)
{
    LinkLayer ll;
    strcpy(ll.serialPort, portName);
    ll.role = (strcmp(mode, "tx") == 0) ? LlTx : LlRx;
    ll.baudRate = baudRate;
    ll.nRetransmissions = maxRetries;
    ll.timeout = customTimeout;

    int fd = llopen(ll);
    if (fd < 0) {
        perror("Connection error\n");
        return;
    }

    switch (ll.role) {
    case LlTx: {
        if (send(dataFileName) < 0) {
            fprintf(stderr, "Failed to send file\n");
            return;
        }
        break;
    }
    case LlRx: {
        if (receive(dataFileName) < 0) {
            fprintf(stderr, "Failed to receive file\n");
            return;
        }
        break;
    }
    }
    llclose(0);
}

