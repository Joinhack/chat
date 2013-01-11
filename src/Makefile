CC=gcc
CFLGAS=-std=c99
OPTS = -lpthread -g 


.c.o:
	$(CC) -c $<  $(CFALGS)

all: server

clean:
	rm *.o
	rm server

server: server.o cnet.o cqueue.o cevent.o jmalloc.o cio.o network.o spinlock.o cthread.o cstr.o
	$(CC) -o server $^ $(OPTS) 