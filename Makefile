.PHONY: all clean

CFLAGS=-Wall -Wextra -O2

OBJ=\
	hbr.o \
	stream_reader.o \
	main.o

all: hbr-dump

hbr-dump: $(OBJ)
	cc *.o -o hbr-dump -lz -lhb -ljq -lm -O2

clean:
	rm -f *.o hbr-dump
