CC = gcc
CFLAGS = -Wall
DEPS = parsing_url.h socket_utils.h clientFTP.h
OBJ = parsing_url.o socket_utils.o clientFTP.o
TARGETS = download

all: download

%.o: %.c $(DEPS) 
	$(CC) $(CFLAGS) -c -o $@ $<

download: $(OBJ)
	@$(CC) $(CFLAGS) -o $@ $@.c $(OBJ) -lm

.PHONY : clean

clean:
	@rm *.o $(TARGETS)
