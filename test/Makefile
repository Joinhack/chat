CC = gcc
INC = ../src
OPTS = -g

TARGS=cstr_test cnet_test cthread_test jmalloc_test cqueue_test dict_test 


all: $(TARGS)

cnet_test:
	$(CC) -o cnet_test cnet_test.c ../src/cnet.c ../src/cio.c ../src/cstr.c ../src/jmalloc.c -I$(INC) $(OPTS)

cstr_test:
	$(CC) -o cstr_test cstr_test.c ../src/cstr.c ../src/jmalloc.c -I$(INC) $(OPTS) -g

jmalloc_test:
	$(CC) -o jmalloc_test jmalloc_test.c -lpthread -DTHREAD_TEST ../src/jmalloc.c -I$(INC) 

cthread_test:
	$(CC) -o cthread_test  cthread_test.c ../src/cthread.c ../src/cqueue.c ../src/spinlock.c ../src/jmalloc.c -I$(INC) $(OPTS) -lpthread 

cqueue_test:
	$(CC) -o cqueue_test cqueue_test.c  ../src/cthread.c ../src/cqueue.c ../src/spinlock.c ../src/jmalloc.c -I$(INC) $(OPTS) -lpthread 

dict_test:
	$(CC) -o dict_test dict_test.c  ../src/dict.c ../src/cstr.c ../src/jmalloc.c -I$(INC)  $(OPTS)

clean: 
	rm -rf *.dSYM $(TARGS)

