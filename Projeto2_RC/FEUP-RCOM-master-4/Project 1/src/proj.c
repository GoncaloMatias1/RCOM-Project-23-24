#include "application.h"
#include <sys/time.h>

int main(int argc, char** argv)
{
    if(argc < 6){
      printf("Usage: serialPort flag file packetSize baudrate\n");
      return -1;
    }

    if ( 
    ((strcmp("/dev/ttyS0", argv[1])!=0) && 
    (strcmp("/dev/ttyS1", argv[1])!=0) && 
    (strcmp("/dev/ttyS10", argv[1])!=0) &&  
    (strcmp("/dev/ttyS11", argv[1])!=0))) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    int fd;
    int flag;
    int packetSize = atoi(argv[4]);

    if(strcmp("1",argv[2]) == 0){
      flag = RECEIVER;
    }
    else if(strcmp("0",argv[2]) == 0){
      flag = TRANSMITTER;
    }
    else{
      printf("Invalid argument for flag\n");
      exit(2);
    }
    
    if(strcmp(argv[5],"B50") != 0 && strcmp(argv[5],"B75") != 0 && strcmp(argv[5],"B110") != 0 && strcmp(argv[5],"B134") != 0 &&
      strcmp(argv[5],"B150") != 0 && strcmp(argv[5],"B200") != 0 && strcmp(argv[5],"B300") != 0 && strcmp(argv[5],"B600") != 0 && strcmp(argv[5],"B1200") != 0 &&
      strcmp(argv[5],"B1800") != 0 && strcmp(argv[5],"B2400") != 0 && strcmp(argv[5],"B4800") != 0 && strcmp(argv[5],"B9600") != 0 && strcmp(argv[5],"B19200") != 0 &&
      strcmp(argv[5],"B38400") != 0 && strcmp(argv[5],"B57600") != 0 && strcmp(argv[5],"B115200") != 0 && strcmp(argv[5],"B230400") != 0)
    {
      printf("Baurate must be one of the following values: B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400\n");
      return -2;
    }

    struct timeval start, end;
    double time_taken;


    gettimeofday(&start, NULL);

    fd = llopen(argv[1],flag, argv[5]);

    applicationSetUp(argv[3],packetSize,fd);

    if(fd > 0){
      if(flag == RECEIVER){
        receiveFile();
      }
      else if(flag == TRANSMITTER){
        sendFile();
      }
      llclose(fd,flag);
    }

    gettimeofday(&end, NULL);
    
    time_taken = end.tv_sec + end.tv_usec / 1e6 -
                  start.tv_sec - start.tv_usec / 1e6; // in seconds

    printf("Time spent: %lf\n",time_taken);
    
    return 0;
}
