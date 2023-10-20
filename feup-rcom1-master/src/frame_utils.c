#include "frame_utils.h"

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
