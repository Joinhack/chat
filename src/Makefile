CC=gcc
CFLAGS=-DUSE_JEMALLOC -g
OPTS = -lpthread -ljemalloc


.c.o:
	$(CC) -c $<  $(CFLAGS)

all: server

clean:
	rm *.o
	rm server

server: server.o cnet.o cqueue.o cevent.o jmalloc.o cio.o network.o spinlock.o cthread.o cstr.o
	$(CC) -o server $^ $(OPTS) 