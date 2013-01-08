CC = gcc
INC = ../src
OPTS = -g

cnet_test:
	$(CC) -o cnet_test cnet_test.c ../src/cnet.c -I$(INC) $(OPTS)

cstr_test:
	$(CC) -o cstr_test cstr_test.c ../src/cstr.c ../src/jmalloc.c -I$(INC) $(OPTS)

all: cstr_test cnet_test

