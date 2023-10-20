#include "link_layer.h"

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "frame_utils.h"
#include "transmitter.h"
#include "receptor.h"

LinkLayerRole role;

int llopen(LinkLayer connectionParameters) {
    if (connectionParameters.role == LlTx) {
        if (open_transmitter(connectionParameters.serialPort,
                    connectionParameters.baudRate,
                    connectionParameters.timeout,
                    connectionParameters.nRetransmissions)) {
            return -1;
        }
        role = LlTx;

        if (connect_trasmitter()) {  // Corrigido o typo aqui
            return -1;
        }
    } else if (connectionParameters.role == LlRx) {
        if (open_receptor(connectionParameters.serialPort, connectionParameters.baudRate)) {
            return -1;
        }
        role = LlRx;

        if (connect_receptor()) {
            return -1;
        }
    }

    return 1;
}

int llwrite(const unsigned char *buf, int bufSize) {
    if (role == LlRx) {
        return 1;
    }

    if (send_packet(buf, bufSize)) {
        return -1;
    }

    printf("Sent packet: ");
    for (int i = 0; i < bufSize; i++) {
        printf("0x%02x ", buf[i]);
    }
    printf("\n");

    return 0;
}

int llread(unsigned char *packet) {
    if (role == LlTx) {
        return 1;
    }

    unsigned char data[DATA_SIZE];
    int size;
    if ((size = receive_packet(data)) < 0) {
        return -1;
    }
    memcpy(packet, data, size);

    printf("Received packet: ");
    for (int i = 0; i < size; i++) {
        printf("0x%02x ", packet[i]);
    }
    printf("\n");

    return size;
}

int llclose(int showStatistics) {
    (void)showStatistics;  // Silenciando o warning de variável não utilizada, se você não estiver usando essa variável

    if (role == LlTx) {
        if (disconnect_trasmitter()) {  // Corrigido o typo aqui
            return -1;
        }

        if (close_transmitter()) {
            return -1;
        }
    } else if (role == LlRx) {
        if (disconnect_receptor()) {
            return -1;
        }

        if (close_receptor()) {
            return -1;
        }
    }

    return 1;
}
