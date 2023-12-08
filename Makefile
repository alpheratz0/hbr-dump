.PHONY: all clean

CC=gcc
CFLAGS=-Wall -Wextra -O2
BIN=hbrdump
RM=/bin/rm
LDFLAGS=-lz -lhb -ljq -lm

OBJ=\
	hbr.o \
	stream_reader.o \
	player.o \
	main.o

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $^ -o $(BIN) $(LDFLAGS)

clean:
	$(RM) -f $(OBJ) $(BIN)
