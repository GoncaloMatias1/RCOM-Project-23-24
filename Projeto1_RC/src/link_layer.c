// Link layer protocol implementation

#include "link_layer.h"

#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <stdio.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

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

        if (connect_transmitter()) {
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
    } else {
        printf("Sent packet: ");
        for (int i = 0; i < bufSize; i++) {
            printf("0x%02x ", buf[i]);
        }
        printf("\n");
    }

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
    // TODO find what is the statistics

    if (role == LlTx) {
        if (disconnect_transmitter()) {
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


//FRAME UTILS

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct data_holder_s data_holder;
struct alarm_config_s alarm_config;

size_t stuff_data(const uint8_t* data, size_t length, uint8_t bcc2, uint8_t* stuffed_data) {
    size_t stuffed_length = 0;

    for (int i = 0; i < length; i++) {
        if (data[i] == FLAG || data[i] == ESC) {
            stuffed_data[stuffed_length++] = ESC;
            stuffed_data[stuffed_length++] = data[i] ^ STUFF_XOR;
        } else {
            stuffed_data[stuffed_length++] = data[i];
        }
    }

    if (bcc2 == FLAG || bcc2 == ESC) {
        stuffed_data[stuffed_length++] = ESC;
        stuffed_data[stuffed_length++] = bcc2 ^ STUFF_XOR;
    } else {
        stuffed_data[stuffed_length++] = bcc2;
    }

    return stuffed_length;
}

size_t destuff_data(const uint8_t* stuffed_data, size_t length, uint8_t* data, uint8_t* bcc2) {
    uint8_t destuffed_data[DATA_SIZE + 1];
    size_t idx = 0;

    for (size_t i = 0; i < length; i++) {
        if (stuffed_data[i] == ESC) {
            i++;
            destuffed_data[idx++] = stuffed_data[i] ^ STUFF_XOR;
        } else {
            destuffed_data[idx++] = stuffed_data[i];
        }
    }

    *bcc2 = destuffed_data[idx - 1];

    memcpy(data, destuffed_data, idx - 1);
    return idx - 1;
}

void build_supervision_frame(int fd, uint8_t address, uint8_t control) {
    data_holder.buffer[0] = FLAG;
    data_holder.buffer[1] = address;
    data_holder.buffer[2] = control;
    data_holder.buffer[3] = address ^ control;
    data_holder.buffer[4] = FLAG;

    data_holder.length = 5;
}

void build_information_frame(int fd, uint8_t address, uint8_t control, const uint8_t* packet, size_t packet_length) {
    uint8_t bcc2 = 0;
    for (size_t i = 0; i < packet_length; i++) {
        bcc2 ^= packet[i];
    }

    uint8_t stuffed_data[STUFFED_DATA_SIZE];
    size_t stuffed_length = stuff_data(packet, packet_length, bcc2, stuffed_data);

    memcpy(data_holder.buffer + 4, stuffed_data, stuffed_length);

    data_holder.buffer[0] = FLAG;
    data_holder.buffer[1] = address;
    data_holder.buffer[2] = control;
    data_holder.buffer[3] = address ^ control;
    data_holder.buffer[4 + stuffed_length] = FLAG;
    data_holder.length = 4 + stuffed_length + 1;
}

int read_supervision_frame(int fd, uint8_t address, uint8_t control, uint8_t* rej_ctrl) {
    uint8_t byte;
    state_t state = START;

    uint8_t is_rej;
    while (state != STOP) {
        if (alarm_config.count > alarm_config.num_retransmissions) {
            return 1;
        }
        if (read(fd, &byte, 1) != 1) {
            continue;
        }
        if (state == START) {
            if (byte == FLAG) {
                state = FLAG_RCV;
            }
        } else if (state == FLAG_RCV) {
            is_rej = 0;
            if (byte == address) {
                state = A_RCV;
            } else if (byte != FLAG) {
                state = START;
            }
        } else if (state == A_RCV) {
            if (rej_ctrl != NULL) {
                if (byte == *rej_ctrl) {
                    is_rej = 1;
                }
            }
            if (byte == control || is_rej) {
                state = C_RCV;
            } else if (byte == FLAG) {
                state = FLAG_RCV;
            } else {
                state = START;
            }
        } else if (state == C_RCV) {
            if ((!is_rej && byte == (address ^ control)) || (is_rej && byte == (address ^ *rej_ctrl))) {
                state = BCC_OK;
            } else if (byte == FLAG) {
                state = FLAG_RCV;
            } else {
                state = START;
            }
        } else if (state == BCC_OK) {
            if (byte == FLAG) {
                if (is_rej) {
                    return 2;
                }
                state = STOP;
            } else {
                state = START;
            }
        }
    }

    return 0;
}

int read_information_frame(int fd, uint8_t address, uint8_t control, uint8_t repeated_ctrl) {
    uint8_t byte;
    state_t state = START;

    uint8_t is_repeated;
    data_holder.length = 0;
    memset(data_holder.buffer, 0, STUFFED_DATA_SIZE + 5);

    while (state != STOP) {
        if (alarm_config.count > alarm_config.num_retransmissions) {
            return 1;
        }
        if (read(fd, &byte, 1) != 1) {
            continue;
        }
        if (state == START) {
            if (byte == FLAG) {
                state = FLAG_RCV;
            }
        } else if (state == FLAG_RCV) {
            is_repeated = 0;
            if (byte == address) {
                state = A_RCV;
            } else if (byte != FLAG) {
                state = START;
            }
        } else if (state == A_RCV) {
            if (byte == repeated_ctrl) {
                is_repeated = 1;
            }
            if (byte == control || is_repeated) {
                state = C_RCV;
            } else if (byte == FLAG) {
                state = FLAG_RCV;
            } else {
                state = START;
            }
        } else if (state == C_RCV) {
            if ((!is_repeated && byte == (address ^ control)) || (is_repeated && byte == (address ^ repeated_ctrl))) {
                state = BCC_OK;
            } else if (byte == FLAG) {
                state = FLAG_RCV;
            } else {
                state = START;
            }
        } else if (state == BCC_OK) {
            if (byte == FLAG) {
                state = STOP;
                if (is_repeated) {
                    return 2;
                }
            } else {
                data_holder.buffer[data_holder.length++] = byte;
            }
        }
    }

    return 0;
}

//END FRAME UTILS


//RECEPTOR


#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>


struct {
    int fd;
    struct termios oldtio, newtio;
} receptor;

int receptor_num = 1;

int open_receptor(char* serial_port, int baudrate) {
    receptor.fd = open(serial_port, O_RDWR | O_NOCTTY);
    if (receptor.fd < 0) {
        return 1;
    }

    if (tcgetattr(receptor.fd, &receptor.oldtio) == -1) {
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
        return 3;
    }

    return 0;
}

int close_receptor() {
    if (tcdrain(receptor.fd) == -1) {
        return 1;
    }

    if (tcsetattr(receptor.fd, TCSANOW, &receptor.oldtio) == -1) {
        return 2;
    }

    close(receptor.fd);
    return 0;
}

int connect_receptor() {
    while (read_supervision_frame(receptor.fd, TX_ADDRESS, SET_CONTROL, NULL) != 0) {}

    build_supervision_frame(receptor.fd, RX_ADDRESS, UA_CONTROL);
    if (write(receptor.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return 1;
    }

    return 0;
}

int disconnect_receptor() {
    while (read_supervision_frame(receptor.fd, TX_ADDRESS, DISC_CONTROL, NULL) != 0) {}

    build_supervision_frame(receptor.fd, RX_ADDRESS, DISC_CONTROL);
    if (write(receptor.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return 1;
    }

    while (read_supervision_frame(receptor.fd, TX_ADDRESS, UA_CONTROL, NULL) != 0) {}

    return 0;
}

int receive_packet(uint8_t* packet) {
    sleep(1);
    if (read_information_frame(receptor.fd, TX_ADDRESS, I_CONTROL(1 - receptor_num), I_CONTROL(receptor_num)) != 0) {
        build_supervision_frame(receptor.fd, RX_ADDRESS, RR_CONTROL(1 - receptor_num));
        if (write(receptor.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
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
        if (write(receptor.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
            return -1;
        }

        return 0;
    }

    memcpy(packet, data, data_size);
    build_supervision_frame(receptor.fd, RX_ADDRESS, RR_CONTROL(receptor_num));
    if (write(receptor.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return -1;
    }

    receptor_num = 1 - receptor_num;
    return data_size;
}


//transmitter


#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>


#include <stdio.h>

struct {
    int fd;
    struct termios oldtio, newtio;
} transmitter;

int transmitter_num = 0;

void alarm_handler(int signo) {
    alarm_config.count++;
    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return ;
    }
    alarm(alarm_config.timeout);

    if (alarm_config.count <= alarm_config.num_retransmissions)
        printf("Alarm #%d\n", alarm_config.count);
}

int open_transmitter(char* serial_port, int baudrate, int timeout, int nRetransmissions) {
    alarm_config.count = 0;
    alarm_config.timeout = timeout;
    alarm_config.num_retransmissions = nRetransmissions;

    (void)signal(SIGALRM, alarm_handler);

    transmitter.fd = open(serial_port, O_RDWR | O_NOCTTY);

    if (transmitter.fd < 0) {
        return 1;
    }

    if (tcgetattr(transmitter.fd, &transmitter.oldtio) == -1) {
        return 2;
    }

    memset(&transmitter.newtio, 0, sizeof(transmitter.newtio));

    transmitter.newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
    transmitter.newtio.c_iflag = IGNPAR;
    transmitter.newtio.c_oflag = 0;

    transmitter.newtio.c_lflag = 0;
    transmitter.newtio.c_cc[VTIME] = 0;
    transmitter.newtio.c_cc[VMIN] = 0;

    tcflush(transmitter.fd, TCIOFLUSH);

    if (tcsetattr(transmitter.fd, TCSANOW, &transmitter.newtio) == -1) {
        return 3;
    }

    return 0;
}

int close_transmitter() {
    if (tcdrain(transmitter.fd) == -1) {
        return 1;
    }

    if (tcsetattr(transmitter.fd, TCSANOW, &transmitter.oldtio) == -1) {
        return 2;
    }

    close(transmitter.fd);
    return 0;
}

int connect_transmitter() {
    alarm_config.count = 0;

    build_supervision_frame(transmitter.fd, TX_ADDRESS, SET_CONTROL);

    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return 1;
    }
    alarm(alarm_config.timeout);

    if (read_supervision_frame(transmitter.fd, RX_ADDRESS, UA_CONTROL, NULL) != 0) {
        alarm(0);
        return 2;
    }
    alarm(0);
    
    return 0;
}

int disconnect_transmitter() {
    alarm_config.count = 0;

    build_supervision_frame(transmitter.fd, TX_ADDRESS, DISC_CONTROL);
    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return 1;
    }
    alarm(alarm_config.timeout);

    int flag = 0;
    for (;;) {
        if (read_supervision_frame(transmitter.fd, RX_ADDRESS, DISC_CONTROL, NULL) == 0) {
            flag = 1;
            break;
        }

        if (alarm_config.count == alarm_config.num_retransmissions) {
            break;
        }
    }
    alarm(0);

    if (!flag) {
        return 2;
    }

    build_supervision_frame(transmitter.fd, TX_ADDRESS, UA_CONTROL);
    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return 3;
    }

    return 0;
}

int send_packet(const uint8_t* packet, size_t length) {
    alarm_config.count = 0;

    build_information_frame(transmitter.fd, TX_ADDRESS, I_CONTROL(transmitter_num), packet, length);
    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return 1;
    }
    alarm(alarm_config.timeout);

    int res = -1;
    uint8_t rej_ctrl = REJ_CONTROL(1 - transmitter_num);

    while (res != 0) {
        res = read_supervision_frame(transmitter.fd, RX_ADDRESS, RR_CONTROL(1 - transmitter_num), &rej_ctrl);
        if (res == 1) {
            break;
        }
    }
    alarm(0);
    if (res == 1) {
        return 2;
    }

    transmitter_num = 1 - transmitter_num;
    return 0;
}