#ifndef _FRAME_UTILS_H_
#define _FRAME_UTILS_H_

#include <stdint.h>
#include <stdlib.h>

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
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    STOP,
} state_t;

struct data_holder_s {
    // +5 for other frame fields (address, control, bcc1, flag) 
    uint8_t buffer[STUFFED_DATA_SIZE + 5];
    size_t length;
};

struct alarm_config_s {
    volatile int count;
    int timeout;
    int num_retransmissions;
};

extern struct data_holder_s data_holder;
extern struct alarm_config_s alarm_config;

size_t stuff_data(const uint8_t* data, size_t length, uint8_t bcc2, uint8_t* stuffed_data);

size_t destuff_data(const uint8_t* stuffed_data, size_t length, uint8_t* data, uint8_t* bcc2);

void build_supervision_frame(int fd, uint8_t address, uint8_t control);

void build_information_frame(int fd, uint8_t address, uint8_t control, const uint8_t* packet, size_t packet_length);

int read_supervision_frame(int fd, uint8_t address, uint8_t control, uint8_t* rej_byte);

int read_information_frame(int fd, uint8_t address, uint8_t control, uint8_t repeated_ctrl);

#endif // _FRAME_UTILS_H_
