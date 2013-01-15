CC = gcc
INC = ../src
OPTS = -std=c99

TARGS=cstr_test cnet_test cthread_test jmalloc_test cqueue_test


all: $(TARGS)

cnet_test:
	$(CC) -o cnet_test cnet_test.c ../src/cnet.c ../src/cio.c ../src/cstr.c ../src/jmalloc.c -I$(INC) $(OPTS)

cstr_test:
	$(CC) -o cstr_test cstr_test.c ../src/cstr.c ../src/jmalloc.c -I$(INC) $(OPTS)

jmalloc_test:
	$(CC) -o jmalloc_test jmalloc_test.c -lpthread -DTHREAD_TEST ../src/jmalloc.c -I$(INC) 

cthread_test:
	$(CC) -o cthread_test  cthread_test.c ../src/cthread.c ../src/cqueue.c ../src/spinlock.c ../src/jmalloc.c -I$(INC) $(OPTS) -lpthread 

cqueue_test:
	$(CC) -o cqueue_test cqueue_test.c  ../src/cthread.c ../src/cqueue.c ../src/spinlock.c ../src/jmalloc.c -I$(INC) $(OPTS) -lpthread 

clean: 
	rm $(TARGS)

