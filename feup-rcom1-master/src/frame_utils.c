#include "frame_utils.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct data_holder_s data_holder;
struct alarm_config_s alarm_config;

size_t stuff_data(const uint8_t* orig_data, size_t len, uint8_t bcc2, uint8_t* new_data) {
    size_t new_length = 0;
    for (int i = 0; i < len; i++) {
        if (orig_data[i] == FLAG || orig_data[i] == ESC) {
            new_data[new_length++] = ESC;
            new_data[new_length++] = orig_data[i] ^ STUFF_XOR;
        } else {
            new_data[new_length++] = orig_data[i];
        }
    }

    if (bcc2 == FLAG || bcc2 == ESC) {
        new_data[new_length++] = ESC;
        new_data[new_length++] = bcc2 ^ STUFF_XOR;
    } else {
        new_data[new_length++] = bcc2;
    }

    return new_length;
}

size_t destuff_data(const uint8_t* orig_data, size_t len, uint8_t* new_data, uint8_t* bcc2) {
    uint8_t destuff_buffer[DATA_SIZE + 1];
    size_t index = 0;

    for (size_t i = 0; i < len; i++) {
        if (orig_data[i] == ESC) {
            destuff_buffer[index++] = orig_data[++i] ^ STUFF_XOR;
        } else {
            destuff_buffer[index++] = orig_data[i];
        }
    }

    *bcc2 = destuff_buffer[index - 1];
    memcpy(new_data, destuff_buffer, index - 1);
    return index - 1;
}

void build_supervision_frame(int fd, uint8_t address, uint8_t control) {
    data_holder.buffer[0] = FLAG;
    data_holder.buffer[1] = address;
    data_holder.buffer[2] = control;
    data_holder.buffer[3] = address ^ control;
    data_holder.buffer[4] = FLAG;
    data_holder.length = 5;
}

void build_information_frame(int fd, uint8_t address, uint8_t control, const uint8_t* data_packet, size_t packet_len) {
    uint8_t bcc2 = 0;
    for (size_t i = 0; i < packet_len; i++) {
        bcc2 ^= data_packet[i];
    }

    uint8_t buffer[STUFFED_DATA_SIZE];
    size_t new_length = stuff_data(data_packet, packet_len, bcc2, buffer);
    memcpy(data_holder.buffer + 4, buffer, new_length);

    data_holder.buffer[0] = FLAG;
    data_holder.buffer[1] = address;
    data_holder.buffer[2] = control;
    data_holder.buffer[3] = address ^ control;
    data_holder.buffer[4 + new_length] = FLAG;

    data_holder.length = 5 + new_length;
}

int read_supervision_frame(int fd, uint8_t address, uint8_t control, uint8_t* rej_ctrl) {
    uint8_t b;
    state_t state = START;

    uint8_t is_rej_flag = 0;
    while (state != STOP) {
        if (alarm_config.count > alarm_config.num_retransmissions) return 1;

        if (read(fd, &b, 1) == 1) {
            switch (state) {
                case START:
                    if (b == FLAG) state = FLAG_RCV;
                    break;
                case FLAG_RCV:
                    if (b == address) {
                        state = A_RCV;
                    } else if (b != FLAG) {
                        state = START;
                    }
                    break;
                case A_RCV:
                    if (rej_ctrl && *rej_ctrl == b) is_rej_flag = 1;
                    state = (b == control || is_rej_flag) ? C_RCV : (b == FLAG ? FLAG_RCV : START);
                    break;
                case C_RCV:
                    if (b == (address ^ (is_rej_flag ? *rej_ctrl : control))) state = BCC_OK;
                    else if (b == FLAG) state = FLAG_RCV;
                    else state = START;
                    break;
                case BCC_OK:
                    if (b == FLAG) {
                        state = STOP;
                        if (is_rej_flag) return 2;
                    } else state = START;
                    break;
                default:
                    state = START;
                    break;
            }
        }
    }

    return 0;
}

int read_information_frame(int fd, uint8_t address, uint8_t control, uint8_t repeated_ctrl) {
    uint8_t b;
    state_t state = START;

    uint8_t repeat_flag = 0;
    data_holder.length = 0;
    memset(data_holder.buffer, 0, STUFFED_DATA_SIZE + 5);

    while (state != STOP) {
        if (alarm_config.count > alarm_config.num_retransmissions) return 1;

        if (read(fd, &b, 1) == 1) {
            switch (state) {
                case START:
                    if (b == FLAG) state = FLAG_RCV;
                    break;
                case FLAG_RCV:
                    if (b == address) {
                        state = A_RCV;
                    } else if (b != FLAG) {
                        state = START;
                    }
                    break;
                case A_RCV:
                    if (b == repeated_ctrl) repeat_flag = 1;
                    state = (b == control || repeat_flag) ? C_RCV : (b == FLAG ? FLAG_RCV : START);
                    break;
                case C_RCV:
                    if (b == (address ^ (repeat_flag ? repeated_ctrl : control))) state = BCC_OK;
                    else if (b == FLAG) state = FLAG_RCV;
                    else state = START;
                    break;
                case BCC_OK:
                    if (b == FLAG) {
                        state = STOP;
                        if (repeat_flag) return 2;
                    } else {
                        data_holder.buffer[data_holder.length++] = b;
                    }
                    break;
                default:
                    state = START;
                    break;
            }
        }
    }

    return 0;
}
