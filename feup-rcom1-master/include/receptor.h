#ifndef _EMISSOR_H_
#define _EMISSOR_H_

#include <stdint.h>

// TODO: Adicionar timeouts para o receptor?

int open_receptor(char* serial_port, int baudrate);

int close_receptor();

int connect_receptor();

int disconnect_receptor();

int receive_packet(uint8_t* packet);

#endif // _EMISSOR_H_
