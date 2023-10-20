// Application layer protocol implementation

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

// --- SEND FILE ---

int send_control_packet(uint8_t control, const char* filename, size_t file_size) {
    size_t filename_length = strlen(filename) + 1;

    if (filename_length > 0xff) {
        perror("Filename length exceeds limit");
        return -1;
    }

    uint8_t packet[5 + filename_length + sizeof(size_t)];
    int packet_index = 0;

    packet[packet_index++] = control;

    packet[packet_index++] = FILE_SIZE;
    packet[packet_index++] = (uint8_t) sizeof(size_t);
    memcpy(&packet[packet_index], &file_size, sizeof(size_t));
    packet_index += sizeof(size_t);

    packet[packet_index++] = FILE_NAME;
    packet[packet_index++] = (uint8_t) filename_length;
    memcpy(&packet[packet_index], filename, filename_length);

    return (llwrite(packet, 5 + filename_length + sizeof(size_t)) != -1) ? 0 : -1;
}

int send_data_packet(const uint8_t* data, size_t data_size) {
    uint8_t packet[3 + data_size];

    packet[0] = CONTROL_DATA;
    packet[1] = (uint8_t) (data_size / OCTET_MULT);
    packet[2] = (uint8_t) (data_size % OCTET_MULT);
    memcpy(&packet[3], data, data_size);

    return (llwrite(packet, 3 + data_size) != -1) ? 0 : -1;
}

int transmit_file(const char* path) {
    FILE* file_ptr = fopen(path, "rb");
    if (!file_ptr) {
        perror("Error opening file");
        return -1;
    }

    fseek(file_ptr, 0, SEEK_END);
    size_t f_size = ftell(file_ptr);
    rewind(file_ptr);

    if (send_control_packet(CONTROL_START, path, f_size) == -1) {
        fclose(file_ptr);
        return -1;
    }

    uint8_t buffer[MAX_PACKET_SIZE - 3];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file_ptr))) {
        if (send_data_packet(buffer, bytes_read) == -1) {
            fclose(file_ptr);
            return -1;
        }
    }

    if (send_control_packet(CONTROL_END, path, f_size) == -1) {
        fclose(file_ptr);
        return -1;
    }

    fclose(file_ptr);
    return 0;
}

// --- RECEIVE FILE ---

int process_control_packet(uint8_t control, uint8_t* buffer, size_t* f_size, char* recv_filename) {
    int read_size;
    if ((read_size = llread(buffer)) < 0) return -1;

    if (buffer[0] != control) return -1;

    size_t offset = 1;
    uint8_t type;
    size_t len;

    while (offset < read_size) {
        type = buffer[offset++];
        len = buffer[offset++];

        if (type == FILE_SIZE) {
            memcpy(f_size, &buffer[offset], len);
            offset += len;
        } else if (type == FILE_NAME) {
            memcpy(recv_filename, &buffer[offset], len);
            offset += len;
        } else {
            return -1;
        }
    }

    return 0;
}

int receive_data(uint8_t* buffer) {
    int read_size = llread(buffer);

    if (read_size < 0 || buffer[0] != CONTROL_DATA) return -1;

    return (buffer[1] * OCTET_MULT + buffer[2]);
}

int receive_file(const char* path) {
    uint8_t buffer[MAX_PACKET_SIZE];
    size_t f_size;
    char recv_filename[MAX_PACKET_SIZE];

    if (process_control_packet(CONTROL_START, buffer, &f_size, recv_filename) < 0) return -1;

    FILE* file_ptr = fopen(path, "wb");
    if (!file_ptr) return -1;

    int bytes_received;

    while ((bytes_received = receive_data(buffer)) > 0) {
        fwrite(&buffer[3], 1, bytes_received, file_ptr);
    }

    fclose(file_ptr);
    return 0;
}

// --- MAIN APPLICATION FUNCTION ---

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    if (!serialPort || strncmp(serialPort, "/dev/ttyS", 9)) {
        perror("Invalid serial port");
        return;
    }

    LinkLayer link_layer;
    strncpy(link_layer.serialPort, serialPort, sizeof(link_layer.serialPort));
    link_layer.role = (!role || strcmp(role, "tx")) ? LlTx : LlRx;
    link_layer.baudRate = baudRate;
    link_layer.nRetransmissions = nTries;
    link_layer.timeout = timeout;

    if (llopen(link_layer) < 0) {
        perror("Connection setup failed");
        return;
    }

    if (link_layer.role == LlTx) {
        if (transmit_file(filename) < 0) perror("File transmission failed");
    } else {
        if (receive_file(filename) < 0) perror("File reception failed");
    }

    if (llclose(0) < 0) perror("Failed to close connection");
}
