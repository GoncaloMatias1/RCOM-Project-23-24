#include "link_layer.h"

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "frame_utils.h"
#include "transmitter.h"
#include "receptor.h"

#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>

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


//recetor

struct {
    int fd;
    struct termios oldtio, newtio;
} receptor;

int receptor_num = 1;

// Função auxiliar para escrever e verificar o tamanho
int safe_write(int fd, const void* buffer, size_t size) {
    int result = write(fd, buffer, size);
    if (result != (int)size) {
        return 1;
    }
    return 0;
}

int open_receptor(char* serial_port, int baudrate) {
    receptor.fd = open(serial_port, O_RDWR | O_NOCTTY);
    if (receptor.fd < 0) {
        return 1;
    }

    if (tcgetattr(receptor.fd, &receptor.oldtio) == -1) {
        close(receptor.fd);
        return 2;
    }

    memset(&receptor.newtio, 0, sizeof(receptor.newtio));

    receptor.newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
    receptor.newtio.c_iflag = IGNPAR;
    receptor.newtio.c_oflag = 0;
    receptor.newtio.c_lflag = 0;
    receptor.newtio.c_cc[VTIME] = 0;
    receptor.newtio.c_cc[VMIN] = 0;

    tcflush(receptor.fd, TCIOFLUSH);

    if (tcsetattr(receptor.fd, TCSANOW, &receptor.newtio) == -1) {
        close(receptor.fd);
        return 3;
    }

    return 0;
}

int close_receptor() {
    if (tcdrain(receptor.fd) == -1) {
        return 1;
    }

    if (tcsetattr(receptor.fd, TCSANOW, &receptor.oldtio) == -1) {
        close(receptor.fd);
        return 2;
    }

    if (close(receptor.fd) < 0) {
        return 3;
    }
    return 0;
}

int connect_receptor() {
    while (read_supervision_frame(receptor.fd, TX_ADDRESS, SET_CONTROL, NULL) != 0) {}

    build_supervision_frame(receptor.fd, RX_ADDRESS, UA_CONTROL);
    return safe_write(receptor.fd, data_holder.buffer, data_holder.length);
}

int disconnect_receptor() {
    while (read_supervision_frame(receptor.fd, TX_ADDRESS, DISC_CONTROL, NULL) != 0) {}

    build_supervision_frame(receptor.fd, RX_ADDRESS, DISC_CONTROL);
    if (safe_write(receptor.fd, data_holder.buffer, data_holder.length)) {
        return 1;
    }

    while (read_supervision_frame(receptor.fd, TX_ADDRESS, UA_CONTROL, NULL) != 0) {}

    return 0;
}

int receive_packet(uint8_t* packet) {
    sleep(1);
    if (read_information_frame(receptor.fd, TX_ADDRESS, I_CONTROL(1 - receptor_num), I_CONTROL(receptor_num)) != 0) {
        build_supervision_frame(receptor.fd, RX_ADDRESS, RR_CONTROL(1 - receptor_num));
        if (safe_write(receptor.fd, data_holder.buffer, data_holder.length)) {
            return -1;
        }

        return 0;
    }

    uint8_t data[DATA_SIZE];
    uint8_t bcc2;
    size_t data_size = destuff_data(data_holder.buffer, data_holder.length, data, &bcc2);

    uint8_t tmp_bcc2 = 0;
    for (size_t i = 0; i < data_size; i++) {
        tmp_bcc2 ^= data[i];
    }

    if (tmp_bcc2 != bcc2) {
        build_supervision_frame(receptor.fd, RX_ADDRESS, REJ_CONTROL(1 - receptor_num));
        if (safe_write(receptor.fd, data_holder.buffer, data_holder.length)) {
            return -1;
        }

        return 0;
    }

    memcpy(packet, data, data_size);
    build_supervision_frame(receptor.fd, RX_ADDRESS, RR_CONTROL(receptor_num));
    if (safe_write(receptor.fd, data_holder.buffer, data_holder.length)) {
        return -1;
    }

    receptor_num = 1 - receptor_num;
    return data_size;
}


//transmitter

#include "transmitter.h"

#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "frame_utils.h"
#include <stdio.h>

struct {
    int fd;
    struct termios oldtio, newtio;
} transmitter;

int transmitter_num = 0;

void alarm_handler(int signo) {
    (void)signo; // silencia o warning de variável não utilizada

    alarm_config.count++;

    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return;
    }
    alarm(alarm_config.timeout);

    if (alarm_config.count <= alarm_config.num_retransmissions)
        printf("Alarm #%d\n", alarm_config.count);
}

int open_transmitter(char* serial_port, int baudrate, int timeout, int nRetransmissions) {
    alarm_config.count = 0;
    alarm_config.timeout = timeout;
    alarm_config.num_retransmissions = nRetransmissions;

    signal(SIGALRM, alarm_handler);

    transmitter.fd = open(serial_port, O_RDWR | O_NOCTTY);
    if (transmitter.fd < 0) return 1;

    if (tcgetattr(transmitter.fd, &transmitter.oldtio) == -1) return 2;

    memset(&transmitter.newtio, 0, sizeof(transmitter.newtio));
    transmitter.newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
    transmitter.newtio.c_iflag = IGNPAR;
    transmitter.newtio.c_oflag = 0;
    transmitter.newtio.c_lflag = 0;
    transmitter.newtio.c_cc[VTIME] = 0;
    transmitter.newtio.c_cc[VMIN] = 0;

    tcflush(transmitter.fd, TCIOFLUSH);

    return (tcsetattr(transmitter.fd, TCSANOW, &transmitter.newtio) == -1) ? 3 : 0;
}

int close_transmitter() {
    if (tcdrain(transmitter.fd) == -1 || tcsetattr(transmitter.fd, TCSANOW, &transmitter.oldtio) == -1) {
        return 1;
    }

    close(transmitter.fd);
    return 0;
}

int connect_trasmitter() {  // Nome da função corrigido
    alarm_config.count = 0;
    build_supervision_frame(transmitter.fd, TX_ADDRESS, SET_CONTROL);
    
    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) return 1;

    alarm(alarm_config.timeout);

    int status = read_supervision_frame(transmitter.fd, RX_ADDRESS, UA_CONTROL, NULL);
    alarm(0);
    return (status != 0) ? 2 : 0;
}

int disconnect_trasmitter() {  // Nome da função corrigido
    alarm_config.count = 0;
    build_supervision_frame(transmitter.fd, TX_ADDRESS, DISC_CONTROL);

    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) return 1;
    
    alarm(alarm_config.timeout);

    int status;
    do {
        status = read_supervision_frame(transmitter.fd, RX_ADDRESS, DISC_CONTROL, NULL);
    } while (status != 0 && alarm_config.count < alarm_config.num_retransmissions);
    alarm(0);

    if (status != 0) return 2;

    build_supervision_frame(transmitter.fd, TX_ADDRESS, UA_CONTROL);
    return (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) ? 3 : 0;
}

int send_packet(const uint8_t* packet, size_t length) {
    alarm_config.count = 0;
    build_information_frame(transmitter.fd, TX_ADDRESS, I_CONTROL(transmitter_num), packet, length);

    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) return 1;
    
    alarm(alarm_config.timeout);

    int status;
    uint8_t rej_ctrl = REJ_CONTROL(1 - transmitter_num);
    do {
        status = read_supervision_frame(transmitter.fd, RX_ADDRESS, RR_CONTROL(1 - transmitter_num), &rej_ctrl);
    } while (status != 0 && alarm_config.count <= alarm_config.num_retransmissions);
    alarm(0);

    if (status != 0) return 2;

    transmitter_num = 1 - transmitter_num;
    return 0;
}
