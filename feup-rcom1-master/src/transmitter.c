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
    alarm_config.count++;
    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return ;
    }
    alarm(alarm_config.timeout);

    // if alarm count is > than num_retransmissions,
    // it will try to write one more time but it will fail
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

int connect_trasmitter() {
    alarm_config.count = 0;

    build_supervision_frame(transmitter.fd, TX_ADDRESS, SET_CONTROL);

    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return 1;
    }
    alarm(alarm_config.timeout);

    // Ask teacher if is 1st try + 3 alarm tries or 3 tries as a whole
    if (read_supervision_frame(transmitter.fd, RX_ADDRESS, UA_CONTROL, NULL) != 0) {
        alarm(0);
        return 2;
    }
    alarm(0);
    
    return 0;
}

int disconnect_trasmitter() {
    alarm_config.count = 0;

    build_supervision_frame(transmitter.fd, TX_ADDRESS, DISC_CONTROL);
    if (write(transmitter.fd, data_holder.buffer, data_holder.length) != data_holder.length) {
        return 1;
    }
    alarm(alarm_config.timeout);

    // TODO: update this to new alarm handling
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

    // if is REJ frame, it will try to send again
    while (res != 0) {
        res = read_supervision_frame(transmitter.fd, RX_ADDRESS, RR_CONTROL(1 - transmitter_num), &rej_ctrl);
        if (res == 1) {
            // alarm count is > than num_retransmissions
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
