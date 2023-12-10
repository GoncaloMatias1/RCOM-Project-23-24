CC = gcc
CFLAGS = -Wall
DEPS = url_parser.h network_socket.h ftp_operations.h
OBJ = url_parser.o network_socket.o ftp_operations.o
TARGETS = download

all: download

%.o: %.c $(DEPS) 
	$(CC) $(CFLAGS) -c -o $@ $<

download: $(OBJ)
	@$(CC) $(CFLAGS) -o $@ $@.c $(OBJ) -lm

.PHONY : clean

clean:
	@rm *.o $(TARGETS)
