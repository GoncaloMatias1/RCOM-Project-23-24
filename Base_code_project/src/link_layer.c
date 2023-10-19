// Link layer protocol implementation

#include "link_layer.h"

int retrans = 0;
int alarmT = FALSE;
int timeout = 0;
int alarmC = 0;
unsigned char tramaTx = 0;
unsigned char tramaRx = 1;


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BAUDRATE 38400
#define MAX_PAYLOAD_SIZE 1000

#define BUF_SIZE 256
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define ESC 0x7D
#define A_ER 0x03
#define A_RE 0x01
#define C_SET 0x03
#define C_DISC 0x0B
#define C_UA 0x07
#define C_RR(Nr) ((Nr << 7) | 0x05)
#define C_REJ(Nr) ((Nr << 7) | 0x01)
#define C_N(Ns) (Ns << 6)


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters) {
    State state = START;
    int serialPortFd = openAndConfigureSerialPort(connectionParameters.serialPort);

    if (serialPortFd < 0) {
        return -1;
    }

    unsigned char receivedByte;
    timeout = connectionParameters.timeout;
    retrans = connectionParameters.nRetransmissions;

    switch (connectionParameters.role) {
        case LlTx: {
            (void)signal(SIGALRM, alarmHandler);

            while (retrans != 0 && state != STOP_R) {
                sendSupervisionFrame(serialPortFd, A_ER, C_SET);

                alarm(connectionParameters.timeout);
                alarmT = FALSE;

                while (alarmT == FALSE && state != STOP_R) {
                    if (read(serialPortFd, &receivedByte, 1) > 0) {
                        switch (state) {
                            case START:
                                if (receivedByte == FLAG) state = FLAG_RCV;
                                break;
                            case FLAG_RCV:
                                if (receivedByte == A_RE) state = A_RCV;
                                else if (receivedByte != FLAG) state = START;
                                break;
                            case A_RCV:
                                if (receivedByte == C_UA) state = C_RCV;
                                else if (receivedByte == FLAG) state = FLAG_RCV;
                                else state = START;
                                break;
                            case C_RCV:
                                if (receivedByte == (A_RE ^ C_UA)) state = BCC1_OK;
                                else if (receivedByte == FLAG) state = FLAG_RCV;
                                else state = START;
                                break;
                            case BCC1_OK:
                                if (receivedByte == FLAG) state = STOP_R;
                                else state = START;
                                break;
                            default:
                                break;
                        }
                    }
                }
                retrans--;
            }

            if (state != STOP_R) {
                return -1;
            }
            break;
        }

        case LlRx: {
            while (state != STOP_R) {
                if (read(serialPortFd, &receivedByte, 1) > 0) {
                    switch (state) {
                        case START:
                            if (receivedByte == FLAG) state = FLAG_RCV;
                            break;
                        case FLAG_RCV:
                            if (receivedByte == A_ER) state = A_RCV;
                            else if (receivedByte != FLAG) state = START;
                            break;
                        case A_RCV:
                            if (receivedByte == C_SET) state = C_RCV;
                            else if (receivedByte == FLAG) state = FLAG_RCV;
                            else state = START;
                            break;
                        case C_RCV:
                            if (receivedByte == (A_ER ^ C_SET)) state = BCC1_OK;
                            else if (receivedByte == FLAG) state = FLAG_RCV;
                            else state = START;
                            break;
                        case BCC1_OK:
                            if (receivedByte == FLAG) state = STOP_R;
                            else state = START;
                            break;
                        default:
                            break;
                    }
                }
            }
            sendSupervisionFrame(serialPortFd, A_RE, C_UA);
            break;
        }

        default:
            return -1;
    }
    return serialPortFd;
}


////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int serialPortFd, const unsigned char *dataBuffer, int dataSize) {
    int frameSize = 6 + dataSize;
    unsigned char *frame = (unsigned char *)malloc(frameSize);
    frame[0] = FLAG;
    frame[1] = A_ER;
    frame[2] = C_N(tramaTx);
    frame[3] = frame[1] ^ frame[2];
    memcpy(frame + 4, dataBuffer, dataSize);
    unsigned char BCC2 = dataBuffer[0];

    for (unsigned int i = 1; i < dataSize; i++) {
        BCC2 ^= dataBuffer[i];
    }

    int framePosition = 4;

    for (unsigned int dataPosition = 0; dataPosition < dataSize; dataPosition++) {
        if (dataBuffer[dataPosition] == FLAG || dataBuffer[dataPosition] == ESC) {
            frame = realloc(frame, ++frameSize);
            frame[framePosition++] = ESC;
        }
        frame[framePosition++] = dataBuffer[dataPosition];
    }

    frame[framePosition++] = BCC2;
    frame[framePosition++] = FLAG;

    int currentTransmition = 0;
    int rejected = 0, accepted = 0;

    while (currentTransmition < retrans) { 
        alarmT = FALSE;
        alarm(timeout);
        rejected = 0;
        accepted = 0;
        while (alarmT == FALSE && !rejected && !accepted) {

            write(serialPortFd, frame, framePosition);
            unsigned char result = readControlFrame(serialPortFd);
            
            if(!result){
                continue;
            }
            else if(result == C_REJ(0) || result == C_REJ(1)) {
                rejected = 1;
            }
            else if(result == C_RR(0) || result == C_RR(1)) {
                accepted = 1;
                tramaTx = (tramaTx+1) % 2;
            }
            else continue;

        }
        if (accepted) break;
        currentTransmition++;
    }

    free(frame);

    if (accepted) {
        return frameSize;
    } else {
        llclose(serialPortFd); // Close the connection on failure
        return -1;
    }
}



////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(int fd, unsigned char *packet) {
    unsigned char byte, cField;
    int i = 0;
    State state = START;

    while (state != STOP_R) {
        if (read(fd, &byte, 1) > 0) {
            switch (state) {
                case START:
                    if (byte == FLAG) state = FLAG_RCV;
                    break;
                case FLAG_RCV:
                    if (byte == A_ER) state = A_RCV;
                    else if (byte != FLAG) state = START;
                    break;
                case A_RCV:
                    if (byte == C_N(0) || byte == C_N(1)) {
                        state = C_RCV;
                        cField = byte;
                    } else if (byte == FLAG) state = FLAG_RCV;
                    else if (byte == C_DISC) {
                        sendSupervisionFrame(fd, A_RE, C_DISC);
                        return 0;
                    } else {
                        state = START;
                    }
                    break;
                case C_RCV:
                    if (byte == (A_ER ^ cField)) state = READING_DATA;
                    else if (byte == FLAG) state = FLAG_RCV;
                    else state = START;
                    break;
                case READING_DATA:
                    if (byte == ESC) state = DATA_FOUND_ESC;
                    else if (byte == FLAG) {
                        unsigned char bcc2 = packet[i - 1];
                        i--;
                        packet[i] = '\0';
                        unsigned char acc = packet[0];

                        for (unsigned int j = 1; j < i; j++)
                            acc ^= packet[j];

                        if (bcc2 == acc) {
                            state = STOP_R;
                            sendSupervisionFrame(fd, A_RE, C_RR(tramaRx));
                            tramaRx = (tramaRx + 1) % 2;
                            return i;
                        } else {
                            printf("Error: retransmission\n");
                            sendSupervisionFrame(fd, A_RE, C_REJ(tramaRx));
                            return -1;
                        };
                    } else {
                        packet[i++] = byte;
                    }
                    break;
                case DATA_FOUND_ESC:
                    state = READING_DATA;
                    if (byte == ESC || byte == FLAG) packet[i++] = byte;
                    else {
                        packet[i++] = ESC;
                        packet[i++] = byte;
                    }
                    break;
                default:
                    break;
            }
        }
    }
    return -1;
}



////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int fd) {
    State state = START; // Initialize the state machine
    unsigned char byte;

    // Set up the signal handler for the alarm
    (void) signal(SIGALRM, alarmHandler);

    // While there are retransmissions left and not in the STOP_R state
    while (retrans != 0 && state != STOP_R) {
        // Send a DISC frame
        sendSupervisionFrame(fd, A_ER, C_DISC);
        
        // Set an alarm and reset the alarm trigger flag
        alarm(timeout);
        alarmT = FALSE;

        // While the alarm is not triggered and not in the STOP_R state
        while (alarmT == FALSE && state != STOP_R) {
            if (read(fd, &byte, 1) > 0) {
                switch (state) {
                    case START:
                        if (byte == FLAG) state = FLAG_RCV;
                        break;
                    case FLAG_RCV:
                        if (byte == A_RE) state = A_RCV;
                        else if (byte != FLAG) state = START;
                        break;
                    case A_RCV:
                        if (byte == C_DISC) state = C_RCV;
                        else if (byte == FLAG) state = FLAG_RCV;
                        else state = START;
                        break;
                    case C_RCV:
                        if (byte == (A_RE ^ C_DISC)) state = BCC1_OK;
                        else if (byte == FLAG) state = FLAG_RCV;
                        else state = START;
                        break;
                    case BCC1_OK:
                        if (byte == FLAG) state = STOP_R;
                        else state = START;
                        break;
                    default:
                        break;
                }
            }
        }

        retrans--;
    }

    // If not in the STOP_R state, return an error
    if (state != STOP_R) return -1;

    // Send UA frame as a positive response
    sendSupervisionFrame(fd, A_ER, C_UA);

    // Close the serial port
    return close(fd);
}


int openAndConfigureSerialPort(const char *serialPort) {
    int fd = open(serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror(serialPort);
        return -1;
    }

    struct termios oldtio, newtio;
    
    if (tcgetattr(fd, &oldtio) == -1) {
        perror("tcgetattr");
        close(fd); // Close the file descriptor on error
        return -1;
    }

    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 1; // Wait for at least one character received

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        close(fd); // Close the file descriptor on error
        return -1;
    }

    return fd;
}

void alarmHandler(int signal) {
    alarmT = TRUE;
    alarmC++;
}

unsigned char readControlFrame(int fd) {
    unsigned char byte, cField = 0;
    State state = START;

    while (state != STOP_R && alarmT == FALSE) {
        if (read(fd, &byte, 1) > 0 || 1) {  
            switch (state) {
                case START:
                    if (byte == FLAG) state = FLAG_RCV;
                    break;
                case FLAG_RCV:
                    if (byte == A_RE) state = A_RCV;
                    else if (byte != FLAG) state = START;
                    break;
                case A_RCV:
                    if (byte == C_RR(0) || byte == C_RR(1) || byte == C_REJ(0) || byte == C_REJ(1) || byte == C_DISC){
                        state = C_RCV;
                        cField = byte;   
                    }
                    else if (byte == FLAG) state = FLAG_RCV;
                    else state = START;
                    break;
                case C_RCV:
                    if (byte == (A_RE ^ cField)) state = BCC1_OK;
                    else if (byte == FLAG) state = FLAG_RCV;
                    else state = START;
                    break;
                case BCC1_OK:
                    if (byte == FLAG){
                        state = STOP_R;
                    }
                    else state = START;
                    break;
                default: 
                    break;
            }
        }
    }
    return cField;
}

int sendSupervisionFrame(int fd, unsigned char A, unsigned char C) {
    unsigned char FRAME[5] = {FLAG, A, C, A ^ C, FLAG};
    ssize_t bytes_written = write(fd, FRAME, 5);

    if (bytes_written == -1) {
        // Handle the error appropriately
        return -1;
    } else if (bytes_written != 5) {
        // Handle the case where not all bytes are written
        return -1;
    }

    return 0; // Return 0 for success
}

