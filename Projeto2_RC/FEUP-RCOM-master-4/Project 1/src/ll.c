#include "ll.h"


ConnectionInfo info;

//struct termios
struct termios oldtio;


void sigAlarmHandler(){
  printf("Timeout!\n");
  info.alarmFlag = 1;
  info.numTries++;
}

void initializeAlarm(){
  struct sigaction sa;
	sa.sa_handler = &sigAlarmHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
  info.alarmFlag = 0;

	sigaction(SIGALRM, &sa, NULL);

  alarm(20); 

}

void disableAlarm(){
  struct sigaction sa;
	sa.sa_handler = NULL;

  sigaction(SIGALRM, &sa, NULL);

  info.alarmFlag = 0;

  alarm(0);
}



speed_t checkBaudrate(char * baudRate){
    long br = strtol(baudRate, NULL, 16);
    switch (br){
        case 0xB50:
            return B50;
        case 0xB75:
            return B75;
        case 0xB110:
            return B110;
        case 0xB134:
            return B134;
        case 0xB150:
            return B150;
        case 0xB200:
            return B200;
        case 0xB300:
            return B300;
        case 0xB600:
            return B600;
        case 0xB1200:
            return B1200;
        case 0xB1800:
            return B1800;
        case 0xB2400:
            return B2400;
        case 0xB4800:
            return B4800;
        case 0xB9600:
            return B9600;
        case 0xB19200:
            return B19200;
        case 0xB38400:
            return B38400;
        case 0xB57600:
            return B57600;
        case 0xB115200:
            return B115200;
        case 0xB230400:
            return B230400;
        default:
          return B38400;
    }
}

void infoSetup(char * baudRate){
    info.alarmFlag = 0;
    info.numTries = 0;
    info.ns = 0;
    info.baudRate = checkBaudrate(baudRate); 
    info.firstTime = 1;
}


int verifyControlByte(unsigned char byte){
  return byte == CONTROL_BYTE_SET || byte == CONTROL_BYTE_DISC || byte == CONTROL_BYTE_UA || byte == CONTROL_BYTE_RR0 || byte == CONTROL_BYTE_RR1 || byte == CONTROL_BYTE_REJ0 || byte == CONTROL_BYTE_REJ1;
}

void responseStateMachine(enum state* currentState, unsigned char byte, unsigned char* controlByte){
    switch(*currentState){
        case START:
            if(byte == FLAG){    //flag
                *currentState = FLAG_RCV;
            }
            break;
        case FLAG_RCV:
            if(byte == ADDRESS_FIELD_CMD){   //acknowlegement
                *currentState = A_RCV;
            }
            else if(byte != FLAG){
              *currentState = START;
            }
            break;
        case A_RCV:
            if(verifyControlByte(byte)){
              *currentState = C_RCV;
              *controlByte = byte;
            }
            else if(byte == FLAG){
                *currentState = FLAG_RCV;
            }
            else{
                *currentState = START;
            }
            break;
        case C_RCV:
            if(byte == (ADDRESS_FIELD_CMD^(*controlByte))){
              *currentState = BCC_OK;
            }        
            else if(byte == FLAG){
              *currentState = FLAG_RCV;
            }    
            else{
              *currentState = START;
            }     
            break;
        case BCC_OK:
            if(byte == FLAG){
              *currentState = STOP;
            }
            else{
              *currentState = START;
            }
            break;
        case STOP:
            break;
        default:
            break;
    }
}

void readReceiverResponse(int fd){
  unsigned char byte;
  enum state state = START;
  unsigned char controlByte;
  while(state != STOP && info.alarmFlag == 0){
      if(read(fd,&byte,1) < 0){
        perror("Error reading byte of the receiver response");
      }
      responseStateMachine(&state,byte,&controlByte);
  }
}

void readTransmitterResponse(int fd){
  unsigned char byte;
  enum state state = START;
  unsigned char controlByte;
  while(state != STOP && info.alarmFlag == 0){
    if(read(fd,&byte,1) < 0){
      perror("Error reading byte of the transmitter response");
    }
    responseStateMachine(&state,byte,&controlByte);
  }
}


int llopen(char *port,int flag, char* baudrate){
    struct termios newtio;
    infoSetup(baudrate);
    printf("BAUD: %d\n", info.baudRate);
    //Open the connection
    int fd;
    fd = open(port, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(port); exit(-1); }

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = info.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 10;   /* inter-unsigned character timer unused */
    newtio.c_cc[VMIN] = 1;   /* blocking read until 5 unsigned chars received */

    cfsetspeed(&newtio,info.baudRate);

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set with baudrate: IN: %d | OUT: %d\n", newtio.c_ispeed, newtio.c_ospeed);

    //First frames' transmission
    if(flag == TRANSMITTER){
      unsigned char controlFrame[5];
      controlFrame[0] = FLAG;
      controlFrame[1] = ADDRESS_FIELD_CMD;
      controlFrame[2] = CONTROL_BYTE_SET;
      controlFrame[3] = controlFrame[1] ^ controlFrame[2];
      controlFrame[4] = FLAG;
      do{
       write(fd,controlFrame,5);  //write SET
       printf("Sent SET\n");  
       info.alarmFlag = 0;
       initializeAlarm();
       readReceiverResponse(fd);  //read UA
       if(info.alarmFlag == 0){
         printf("Received UA\n");
       }                    
      } while(info.numTries < MAX_TRIES && info.alarmFlag);

      disableAlarm();

      if(info.numTries >= MAX_TRIES){
        printf("Exceeded number of maximum tries!\n");
        return -1;
      }
    }

    else if(flag == RECEIVER){
      readTransmitterResponse(fd);
      printf("Received SET\n");

      unsigned char controlFrame[5];
      controlFrame[0] = FLAG;
      controlFrame[1] = ADDRESS_FIELD_CMD;
      controlFrame[2] = CONTROL_BYTE_UA;
      controlFrame[3] = controlFrame[1] ^ controlFrame[2];
      controlFrame[4] = FLAG;
      write(fd,controlFrame,5);  //send UA
      printf("Sent UA\n");
    }

    else{
        printf("Unknown function, must be a TRANSMITTER/RECEIVER\n");
        return 1;
    }

    return fd;
}

int processControlByte(int fd, unsigned char *controlByte){
    unsigned char byte;
    enum state state = START;
    while(state != STOP && info.alarmFlag == 0){
        if(read(fd,&byte,1) < 0){
          perror("Error reading byte");
        }
        responseStateMachine(&state,byte,controlByte);
    }

    if(*controlByte == CONTROL_BYTE_RR0 && info.ns == 1){
      printf("Received postive ACK 0\n");
      return 0;
    }
    else if(*controlByte == CONTROL_BYTE_RR1 && info.ns == 0){
      printf("Received postive ACK 1\n");
      return 0;
    }
    else if(*controlByte == CONTROL_BYTE_REJ0 && info.ns == 1){
      printf("Received negative ACK 0\n");
      return -1;
    }
    else if(*controlByte == CONTROL_BYTE_REJ1 && info.ns == 0){
      printf("Received negative ACK 1\n");
      return -1;
    }
    else
    {
      return -1;
    }
    
    return 0;
}


int llwrite(int fd, unsigned char* buffer, int length){
    int unsigned charactersWritten = 0;
    unsigned char controlByte;
    info.numTries = 0;
    if(info.ns == 0 && !info.firstTime)
      info.ns = 1;
    else if(info.ns == 1)
      info.ns = 0;
    do{
      //write frame
      unsigned char frameToSend[2*length+7];
      int frameIndex = 4, frameLength = 0;
      unsigned char bcc2 = 0x00;

      //Start to make the frame to be sent
      frameToSend[0] = FLAG;    //FLAG
      frameToSend[1] = ADDRESS_FIELD_CMD;    //UA
      if(info.ns == 0)
        frameToSend[2] = CONTROL_BYTE_0;
      else
      {
        frameToSend[2] = CONTROL_BYTE_1;
      }
      frameToSend[3] = frameToSend[1] ^ frameToSend[2]; 
      
      for (size_t i = 0; i < length; i++){
        bcc2 ^= buffer[i];
        if(buffer[i] == FLAG || buffer[i] == ESC_BYTE){   //if the byte its equal to the flag or to the escape byte
          frameToSend[frameIndex] = ESC_BYTE;
          frameIndex++;
          frameToSend[frameIndex] = buffer[i] ^ STUFFING_BYTE;
          frameIndex++;
        }
        else{
          frameToSend[frameIndex] = buffer[i];
          frameIndex++;
        }
      }

      if(bcc2 == FLAG || bcc2 == ESC_BYTE){
        frameToSend[frameIndex] = ESC_BYTE;
        frameIndex++;
        frameToSend[frameIndex] = bcc2 ^ STUFFING_BYTE;
        frameIndex++;
      }
      else{
        frameToSend[frameIndex] = bcc2;
        frameIndex++;
      }

      frameToSend[frameIndex] = FLAG;

      frameLength = frameIndex+1;
      printf("Before writing to serial...\n");
      charactersWritten = write(fd,frameToSend,frameLength);
      printf("Sent frame with sequence number %d\n\n",info.ns);

      initializeAlarm();
      
      //read receiver response
      if(processControlByte(fd,&controlByte) == -1){  // if there is an error sending the message, send again 
        disableAlarm();
        info.alarmFlag = 1;
      }

    }while(info.numTries < MAX_TRIES && info.alarmFlag);
    info.firstTime = 0;
    disableAlarm();
    
    if(info.numTries >= MAX_TRIES){
        printf("Exceeded number of maximum tries!\n");
        return -1;
    }

    return charactersWritten; 
}

void informationFrameStateMachine(enum state* currentState, unsigned char byte, unsigned char* controlByte){
    switch(*currentState){
        case START:
            if(byte == FLAG)     //flag
                *currentState = FLAG_RCV;
            break;
        case FLAG_RCV:
            if(byte == ADDRESS_FIELD_CMD)   //acknowlegement
                *currentState = A_RCV;
            else if(byte != FLAG)
                *currentState = START;
            break;
        case A_RCV:
            if(byte == CONTROL_BYTE_0 || byte == CONTROL_BYTE_1){
              *currentState = C_RCV;
              *controlByte = byte;
            }
            else if(FLAG == 0x7E){
                *currentState = FLAG_RCV;
            }
            else{
                *currentState = START;
            }
            break;
        case C_RCV:
            if(byte == (ADDRESS_FIELD_CMD^(*controlByte)))
              *currentState = BCC_OK;
            else if(byte == FLAG)
              *currentState = FLAG_RCV;
            else
              *currentState = START;
            
            break;
        case BCC_OK:
            if(byte != FLAG)
              *currentState = DATA_RCV;
            break;
        case DATA_RCV:
            if(byte == FLAG)
              *currentState = STOP;
            break;
        case STOP:
            break;
    }
}


int readTransmitterFrame(int fd, unsigned char * buffer){
    int length = 0;
    unsigned char byte;
    unsigned char controlByte;
    enum state state = START;
    while(state != STOP){
      if(read(fd,&byte,1) < 0){
          perror("Error reading byte");
      }
      informationFrameStateMachine(&state,byte,&controlByte);
      buffer[length] = byte;
      length++;
    }
    return length;
}

int verifyFrame(unsigned char* frame,int length){
  unsigned char addressField = frame[1];
  unsigned char controlByte = frame[2];
  unsigned char bcc1 = frame[3];
  unsigned char bcc2 = frame[length-2];
  unsigned char aux = 0x00;

  //verify if the bcc1 is correct
  if(controlByte != CONTROL_BYTE_0 && controlByte != CONTROL_BYTE_1){
    printf("Error in control byte!\n");
    return -1;
  }
  else if(bcc1 == (addressField^controlByte)){
    //check bcc2
    for (size_t i = 4; i < length-2; i++)
    {
      aux^=frame[i];
    }
    if(bcc2 != aux){
      printf("Error in bcc2!\n");
      return -2;
    }
  }
  
  return 0;
}

int llread(int fd,unsigned char* buffer){
  int received = 0;
  int length = 0;
  unsigned char controlByte;
  unsigned char auxBuffer[131087];
  int buffIndex = 0;
  info.numTries = 0;
  while(received == 0){
    length = readTransmitterFrame(fd,auxBuffer);
    printf("Received frame\n");

    if(length > 0){
      unsigned char originalFrame[2*length+7];
      //destuffing
      
      originalFrame[0] = auxBuffer[0];
      originalFrame[1] = auxBuffer[1];
      originalFrame[2] = auxBuffer[2];
      originalFrame[3] = auxBuffer[3];
      
      int originalFrameIndex = 4;
      int escapeByteFound = 0;

      for (size_t i = 4; i < length-1; i++)
      { 
        if(auxBuffer[i] == ESC_BYTE){
          escapeByteFound = 1;
          continue;
        }
        else if(auxBuffer[i] == (FLAG^STUFFING_BYTE) && escapeByteFound == 1){
          originalFrame[originalFrameIndex] = FLAG;
          originalFrameIndex++;
          escapeByteFound = 0;
        }
        else if(auxBuffer[i] == (ESC_BYTE^STUFFING_BYTE) && escapeByteFound == 1){
          originalFrame[originalFrameIndex] = ESC_BYTE;
          originalFrameIndex++;
          escapeByteFound = 0;
        }
        else{
          originalFrame[originalFrameIndex] = auxBuffer[i];
          originalFrameIndex++;
        }
      }
      originalFrame[originalFrameIndex] = auxBuffer[length-1];
      controlByte = originalFrame[2];

      if(verifyFrame(originalFrame,originalFrameIndex+1) != 0){
        if(controlByte == CONTROL_BYTE_0){
          printf("Frame has 0 as sequence number\n");
          unsigned char frameToSend[5];
          frameToSend[0] = FLAG;
          frameToSend[1] = ADDRESS_FIELD_CMD;
          frameToSend[2] = CONTROL_BYTE_REJ0;
          frameToSend[3] = frameToSend[1] ^ frameToSend[2];
          frameToSend[4] = FLAG;
          write(fd,frameToSend,5);
          printf("Sent negative ACK 0\n");
        }
        else if(controlByte == CONTROL_BYTE_1){
          printf("Frame has 1 as sequence number\n");
          unsigned char frameToSend[5];
          frameToSend[0] = FLAG;
          frameToSend[1] = ADDRESS_FIELD_CMD;
          frameToSend[2] = CONTROL_BYTE_REJ1;
          frameToSend[3] = frameToSend[1] ^ frameToSend[2];
          frameToSend[4] = FLAG;
          write(fd,frameToSend,5);
          printf("Sent negative ACK 1\n");
        }
        return 0;
      }
      else{
        for (size_t i = 4; i < originalFrameIndex-1; i++)
        {
          buffer[buffIndex] = originalFrame[i];
          buffIndex++;
        }
        if(controlByte == CONTROL_BYTE_0){
          printf("Frame has 0 as sequence number\n");
          unsigned char frameToSend[5];
          frameToSend[0] = FLAG;
          frameToSend[1] = ADDRESS_FIELD_CMD;
          frameToSend[2] = CONTROL_BYTE_RR1;
          frameToSend[3] = frameToSend[1] ^ frameToSend[2];
          frameToSend[4] = FLAG;
          write(fd,frameToSend,5);
          printf("Sent positive ACK 1\n");
        }
        else if(controlByte == CONTROL_BYTE_1){
          printf("Frame has 1 as sequence number\n");
          unsigned char frameToSend[5];
          frameToSend[0] = FLAG;
          frameToSend[1] = ADDRESS_FIELD_CMD;
          frameToSend[2] = CONTROL_BYTE_RR0;
          frameToSend[3] = frameToSend[1] ^ frameToSend[2];
          frameToSend[4] = FLAG;
          write(fd,frameToSend,5);
          printf("Sent positive ACK 0\n");
        }
        received = 1;       
      }
    }
  }
      
  return buffIndex;
}

int llclose(int fd, int flag){
    //Last frames' transmission
    if(flag == TRANSMITTER){
        if(info.numTries >= MAX_TRIES){
            return -1;
        }
        unsigned char controlFrameDISC[5];
        controlFrameDISC[0] = FLAG;
        controlFrameDISC[1] = ADDRESS_FIELD_CMD;
        controlFrameDISC[2] = CONTROL_BYTE_DISC;
        controlFrameDISC[3] = controlFrameDISC[1] ^ controlFrameDISC[2];
        controlFrameDISC[4] = FLAG;
        do{
            write(fd,controlFrameDISC,5);  //write DISC 
            printf("Sent DISC\n");
            info.alarmFlag = 0;
            initializeAlarm();
            readReceiverResponse(fd);  //read DISC
        } while(info.numTries < MAX_TRIES && info.alarmFlag);

        if(info.alarmFlag == 0){
          printf("Received DISC\n");
        }
        
        disableAlarm();

        if(info.numTries >= MAX_TRIES){
            printf("Exceeded number of maximum tries!\n");
            return -1;
        }
        else{
            unsigned char controlFrameUA[5];
            controlFrameUA[0] = FLAG;
            controlFrameUA[1] = ADDRESS_FIELD_CMD;
            controlFrameUA[2] = CONTROL_BYTE_UA;
            controlFrameUA[3] = controlFrameUA[1] ^ controlFrameUA[2];
            controlFrameUA[4] = FLAG;

            write(fd,controlFrameUA,5); //write UA after receiving DISC
            printf("Sent UA\n");
            sleep(1);
        }
    }

    else if(flag == RECEIVER){
        if(info.numTries >= MAX_TRIES){
            return -1;
        }

        readTransmitterResponse(fd); 
        printf("Receveid DISC\n"); 

        unsigned char controlFrame[5];
        controlFrame[0] = FLAG;
        controlFrame[1] = ADDRESS_FIELD_CMD;
        controlFrame[2] = CONTROL_BYTE_DISC;
        controlFrame[3] = controlFrame[1] ^ controlFrame[2];
        controlFrame[4] = FLAG;

        write(fd,controlFrame,5);  //send DISC
        printf("Sent DISC\n");

        readTransmitterResponse(fd); 
        printf("Received UA\n");

    }

    else{
        printf("Unknown function, must be a TRANSMITTER/RECEIVER\n");
        return 1;
    }
    //Close the connection

    tcflush(fd, TCIOFLUSH);

	if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);

  return 0;
}