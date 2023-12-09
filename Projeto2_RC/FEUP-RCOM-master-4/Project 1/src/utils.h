#include <termios.h>


#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define TRANSMITTER 0
#define RECEIVER 1


#define FLAG 0x7E
#define ADDRESS_FIELD_CMD 0x03
#define CONTROL_BYTE_0   0x00
#define CONTROL_BYTE_1   0x40
#define CONTROL_BYTE_SET 0x03
#define CONTROL_BYTE_DISC 0x0B
#define CONTROL_BYTE_UA 0x07
#define CONTROL_BYTE_RR0 0x05
#define CONTROL_BYTE_RR1 0x85
#define CONTROL_BYTE_REJ0 0x01
#define CONTROL_BYTE_REJ1 0x81

#define ESC_BYTE 0x7D
#define STUFFING_BYTE 0x20

#define START_FLAG 0x02
#define END_FLAG 0x03
#define FILE_NAME_FLAG 0x01
#define FILE_SIZE_FLAG 0x00
#define DATA_FLAG 0x01
#define BYTE_MASK 0xFF


#define MAX_TRIES 3

enum state{START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP, DATA_RCV};