#include "receptor.h"

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "frame_utils.h"

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
