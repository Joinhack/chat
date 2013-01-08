CC = gcc
INC = ../src
OPTS = -g

cnet_test:
	$(CC) -o cnet_test cnet_test.c ../src/cnet.c -I$(INC) $(OPTS)

cstr_test:
	$(CC) -o cstr_test cstr_test.c ../src/cstr.c ../src/jmalloc.c -I$(INC) $(OPTS)

jmalloc_test:
	$(CC) -o jmalloc_test jmalloc_test.c -lpthread -DTHREAD_TEST ../src/jmalloc.c -I$(INC) 

all: cstr_test cnet_test

