CC=gcc
CFLAGS= -g -std=gnu99 -DUSE_DEBUG
OPTS = -lpthread 


.c.o:
	$(CC) -c $<  $(CFLAGS)

all: server

clean:
	rm -rf *.o
	rm -rf server

server: server.o cnet.o cqueue.o cevent.o jmalloc.o cio.o network.o spinlock.o cthread.o cstr.o log.o common.o
	$(CC) -o server $^ $(OPTS) 
