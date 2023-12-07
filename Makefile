.PHONY: all clean

CFLAGS=-Wall -Wextra -O2

OBJ=\
	hbr.o \
	stream_reader.o \
	main.o

all: hbrdump

hbrdump: $(OBJ)
	cc *.o -o hbrdump -lz -lhb -ljq -lm -O2

clean:
	rm -f *.o hbrdump
