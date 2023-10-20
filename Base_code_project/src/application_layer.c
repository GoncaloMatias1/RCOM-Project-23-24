// Application layer protocol implementation

#include <stdio.h>
#include <math.h>
#include "application_layer.h"

enum ControlPacketType {
    CONTROL_START,
    CONTROL_END
};


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
        if (send(filename) < 0) {
            printf(stderr, "Failed to send file\n");
            return;
        }
        break;
    }
    case LlRx: {
        if (receive(filename) < 0) {
            printf(stderr, "Failed to receive file\n");
            return;
        break;
        }
    }
    }
    llclose(0);
}

int open_file(const char* filename) {
    FILE* fs;
    if (!filename) {
        fprintf(stderr, "Invalid filename\n");
        return -1;
    }

    fs = fopen(filename, "rb");
    if (fs == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        return -1;
    }
    return (int)fs; 
}

void close_file(int file_handle) {
    if (file_handle >= 0) {
        fclose((FILE*)file_handle);
    }
}

size_t read_file(int file_handle, uint8_t* buf, size_t size) {
    if (file_handle < 0) {
        return 0;
    }
    return fread(buf, 1, size, (FILE*)file_handle);
}

int send_file(const char* filename) {
    int file_handle = open_file(filename);
    if (file_handle < 0) {
        return -1;
    }

    fseek((FILE*)file_handle, 0, SEEK_END);
    size_t file_size = ftell((FILE*)file_handle);
    fseek((FILE*)file_handle, 0, SEEK_SET);

    if (send_control_packet(CONTROL_START, filename, file_size) == -1) {
        fprintf(stderr, "Failed to send start control packet\n");
        close_file(file_handle);
        return -1;
    }

    uint8_t buf[MAX_PACKET_SIZE - 3];
    size_t bytes_read;
    while ((bytes_read = read_file(file_handle, buf, MAX_PACKET_SIZE - 3)) > 0) {
        size_t octets = bytes_read / OCTET_MULT;
        size_t remainder = bytes_read % OCTET_MULT;

        // Ensure octets and remainder fit within a byte
        if (octets > 0xFF || remainder > 0xFF) {
            fprintf(stderr, "Data packet too large\n");
            close_file(file_handle);
            return -1;
        }

        size_t packet_length = 3 + bytes_read;
        uint8_t packet[packet_length];

        // Control code for data packet
        packet[0] = CONTROL_DATA;
        
        // Number of octets (high and low bytes)
        packet[1] = (uint8_t) octets;
        packet[2] = (uint8_t) remainder;

        // Copy the data into the packet
        memcpy(packet + 3, buf, bytes_read);

        // Replace this placeholder with your logic to send the data packet
        if (llwrite(packet, packet_length) == -1) {
            fprintf(stderr, "Failed to send data packet\n");
            close_file(file_handle);
            return -1;
        }
    }

    if (send_control_packet(CONTROL_END, filename, file_size) == -1) {
        fprintf(stderr, "Failed to send end control packet\n");
        close_file(file_handle);
        return -1;
    }

    close_file(file_handle);
    return 0;
}

int send_control_packet(enum ControlPacketType type, const char* filename, size_t file_size) {
    uint8_t control;
    if (type == CONTROL_START) {
        control = CONTROL_START;
    } else if (type == CONTROL_END) {
        control = CONTROL_END;
    } else {
        fprintf(stderr, "Invalid control type\n");
        return -1;
    }

    size_t filename_length = strlen(filename) + 1;
    if (filename_length > 0xFF) {
        fprintf(stderr, "Filename too long (needs to fit in a byte, including '\\0')\n");
        return -1;
    }

    size_t packet_length = 5 + filename_length + sizeof(size_t);
    uint8_t packet[packet_length];

    packet[0] = control;

    if (type == CONTROL_START || type == CONTROL_END) {
        packet[1] = FILE_SIZE;
        packet[2] = (uint8_t)sizeof(size_t);
        memcpy(packet + 3, &file_size, sizeof(size_t));
    } else {
        packet[1] = FILE_NAME;
        packet[2] = (uint8_t)filename_length;
        memcpy(packet + 3, filename, filename_length);
    }

    // Replace this placeholder with your logic to send the packet
    if (llwrite(packet, packet_length) == -1) {
        fprintf(stderr, "Failed to send control packet\n");
        return -1;
    }
    return 0;
}

int send_control_packet(uint8_t control, const char* filename, size_t file_size) {
    size_t filename_length = strlen(filename) + 1;
    if (filename_length > 0xff) {
        fprintf(stderr, "Filename too long (needs to fit in a byte, including '\\0')\n");
        return -1;
    }

    size_t packet_length = 5 + filename_length + sizeof(size_t);
    uint8_t packet[packet_length];

    packet[0] = control;

    packet[1] = FILE_SIZE;
    packet[2] = (uint8_t) sizeof(size_t);
    memcpy(packet + 3, &file_size, sizeof(size_t));

    packet[3 + sizeof(size_t)] = FILE_NAME;
    packet[4 + sizeof(size_t)] = (uint8_t) filename_length;
    memcpy(packet + 5 + sizeof(size_t), filename, filename_length);

    if (llwrite(packet, packet_length) == -1) {
        fprintf(stderr, "Failed to send control packet\n");
        return -1;
    }
    return 0;
}


int read_control_packet(uint8_t control, uint8_t* buf, size_t* file_size, char* received_filename) {
    if (file_size == NULL) {
        fprintf(stderr, "Invalid file size pointer\n");
        return -1;
    }

    int size;
    if ((size = llread(buf)) < 0) {
        fprintf(stderr, "Failed to read control packet\n");
        return -1;
    }

    if (buf[0] != control) {
        fprintf(stderr, "Invalid control packet\n");
        return -1;
    }

    uint8_t type;
    size_t length;
    size_t offset = 1;

    while (offset < size) {
        type = buf[offset++];
        if (type != FILE_SIZE && type != FILE_NAME) {
            fprintf(stderr, "Invalid control packet type\n");
            return -1;
        }

        if (type == FILE_SIZE) {
            length = buf[offset++];
            if (length != sizeof(size_t)) {
                fprintf(stderr, "Invalid file size length\n");
                return -1;
            }
            memcpy(file_size, buf + offset, sizeof(size_t));
            offset += sizeof(size_t);
        } else {
            length = buf[offset++];
            if (length > MAX_PACKET_SIZE - offset) {
                fprintf(stderr, "Invalid file name length\n");
                return -1;
            }
            memcpy(received_filename, buf + offset, length);
            offset += length;
        }
    }

    return 0;
}

int receive_file(const char* filename) {
    uint8_t buf[MAX_PACKET_SIZE];
    size_t file_size;

    // TODO: ask teacher if we should use args filename or filename from control packet
    char received_filename[0xff];

    if (read_control_packet(CONTROL_START, buf, &file_size, received_filename) == -1) {
        fprintf(stderr, "Failed to read start control packet\n");
        return -1;
    }

    FILE* fs;
    if ((fs = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "Error opening file\n");
        return -1;
    }

    int size;
    while ((size = llread(buf)) > 0) {
        if (buf[0] == CONTROL_END) {
            break;
        }

        if (buf[0] != CONTROL_DATA) {
            fprintf(stderr, "Invalid data packet\n");
            return -1;
        }

        size_t length = buf[1] * OCTET_MULT + buf[2];
        uint8_t* data = (uint8_t*)malloc(length);
        memcpy(data, buf + 3, size - 3);

        if (fwrite(data, sizeof(uint8_t), length, fs) != length) {
            fprintf(stderr, "Failed to write to file\n");
            return -1;
        }

        free(data);
    }

    fclose(fs);
    return 0;
}
