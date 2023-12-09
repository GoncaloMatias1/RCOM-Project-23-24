CC = gcc
CFLAGS = -Wall
DEPS = url.h socket.h ftp.h
OBJ = url.o socket.o ftp.o
TARGETS = download

all: download

%.o: %.c $(DEPS) 
	$(CC) $(CFLAGS) -c -o $@ $<

download: $(OBJ)
	@$(CC) $(CFLAGS) -o $@ $@.c $(OBJ) -lm

.PHONY : clean

clean:
	@rm *.o $(TARGETS)
