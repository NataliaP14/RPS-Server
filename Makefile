CC = gcc
CFLAGS = -g -Wall -std=c99 -fsanitize=address,undefined -pthread

OBJS = network.o

all: rpsd 

rpsd: rpsd.o $(OBJS)
	$(CC) $(CFLAGS) -o rpsd rpsd.o $(OBJS)

rpsd.o: rpsd.h network.h
network.o: network.h

clean:
	rm -f *.o rpsd 