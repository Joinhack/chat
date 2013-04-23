CC=gcc
CFLAGS= -g -std=gnu99 -DUSE_TRACE
LIBS = -lpthread 

.c.o:

all: server

clean:
	rm -rf *.o
	rm -rf server

server: server.o cnet.o clist.o cevent.o jmalloc.o cio.o network.o spinlock.o cthread.o cstr.o log.o common.o dict.o db.o obj.o db.o dump.o
	$(CC) -o server $^ $(LIBS)
