CC = gcc
OPTS = -lpthread

all: server

server:
	$(CC) -o server server.c cnet.c  cqueue.c cevent.c jmalloc.c cio.c  network.c spinlock.c cthread.c $(OPTS) 