CC = gcc
CFLAGS = -Wall
DEPS = clientTCP.h getip.h 
OBJ = clientTCP.o getip.o 
TARGETS = download

all: download

%.o: %.c $(DEPS) 
	$(CC) $(CFLAGS) -c -o $@ $<

download: $(OBJ)
	@$(CC) $(CFLAGS) -o $@ $@.c $(OBJ) -lm

.PHONY : clean

clean:
	@rm *.o $(TARGETS)
