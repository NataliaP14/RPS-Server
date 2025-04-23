CC = gcc
CFLAGS = -g -Wall -std=c99 -fsanitize=address,undefined -pthread

OBJS = network.o

all: rpsd rc

rpsd: rpsd.o $(OBJS)
	$(CC) $(CFLAGS) -o rpsd rpsd.o $(OBJS)

rc: rc.o $(OBJS)
	$(CC) $(CFLAGS) -o rc rc.o $(OBJS)

rpsd.o: network.h
rc.o: network.h
network.o: network.h

clean:
	rm -f *.o rpsd rc