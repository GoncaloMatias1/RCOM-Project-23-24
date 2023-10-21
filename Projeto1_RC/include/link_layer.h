// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

// ================================
// _TRANSMITTER_H_ content starts
// ================================

#include <stdlib.h>
#include <stdint.h>

#define MAX_PAYLOAD_SIZE 1000
#define FALSE 0
#define TRUE 1
#define FLAG            0x7E
#define TX_ADDRESS      0x03
#define RX_ADDRESS      0x01
#define SET_CONTROL     0x03
#define UA_CONTROL      0x07
#define RR_CONTROL(n)   ((n == 0) ? 0x05 : 0x85)
#define REJ_CONTROL(n)  ((n == 0) ? 0x01 : 0x81)
#define DISC_CONTROL    0x0B
#define I_CONTROL(n)    ((n == 0) ? 0x00 : 0x40)
#define ESC             0x7D
#define STUFF_XOR       0x20

#define DATA_SIZE           1024
#define STUFFED_DATA_SIZE   (DATA_SIZE * 2 + 2)

typedef enum {
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct {
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    STOP,
} state;

extern struct data_holder {
    uint8_t buffer[STUFFED_DATA_SIZE + 5];
    size_t length;
} dh;

extern struct alarm_config_s {
    volatile int count;
    int timeout;
    int num_retransmissions;
} alarm_config;

int llopen(LinkLayer connectionParameters);
int llwrite(const unsigned char *buf, int bufSize);
int llread(unsigned char *packet);
int llclose(int showStatistics);
size_t transmission_stuff_data(const uint8_t* data, size_t length, uint8_t bcc2, uint8_t* stuffed_data);
size_t transmission_destuff_data(const uint8_t* stuffed_data, size_t length, uint8_t* data, uint8_t* bcc2);
void supervision_frame_aux(int fd, uint8_t address, uint8_t control);
void buildInformation(int fd, uint8_t address, uint8_t control, const uint8_t* packet, size_t packet_length);
int readSupervision(int fd, uint8_t address, uint8_t control, uint8_t* rej_byte);
int readInformation(int fd, uint8_t address, uint8_t control, uint8_t repeated_ctrl);
int open_receptor(char* serial_port, int baudrate);
int close_receptor();
int connect_receptor();
int disconnect_receptor();
int receive_packet(uint8_t* packet);
int open_transmitter(char* serial_port, int baudrate, int timeout, int nRetransmissions);
int close_transmitter();
int connect_transmitter();
int disconnect_transmitter();
int send_packet(const uint8_t* packet, size_t length);



#endif // _LINK_LAYER_H_
