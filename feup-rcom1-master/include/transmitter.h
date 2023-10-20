#ifndef _TRANSMITTER_H_
#define _TRANSMITTER_H_

#include <stdlib.h>
#include <stdint.h>

int open_transmitter(char* serial_port, int baudrate, int timeout, int nRetransmissions);

int close_transmitter();

int connect_trasmitter();

int disconnect_trasmitter();

int send_packet(const uint8_t* packet, size_t length);

#endif // _TRANSMITTER_H_
