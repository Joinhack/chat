CC = clang
INC = ../src
OPTS = -g

TARGS=cstr_test cnet_test cthread_test jmalloc_test clist_test dict_test common_test db_test timer_test


all: $(TARGS)

cnet_test:../src/cnet.c ../src/cio.c ../src/cstr.c ../src/obj.c ../src/dict.c ../src/spinlock.c ../src/jmalloc.c ../src/timer.c ../src/clist.c
	$(CC) -o $@ cnet_test.c $^ -I$(INC) $(OPTS)

cstr_test:
	$(CC) -o $@ cstr_test.c ../src/cstr.c ../src/jmalloc.c -I$(INC) $(OPTS) -g

jmalloc_test:
	$(CC) -o $@ jmalloc_test.c -lpthread -DTHREAD_TEST ../src/jmalloc.c -I$(INC) 

cthread_test:
	$(CC) -o $@  cthread_test.c ../src/cthread.c ../src/clist.c ../src/log.c ../src/spinlock.c ../src/jmalloc.c -I$(INC) $(OPTS) -lpthread 

clist_test:
	$(CC) -o $@ clist_test.c  ../src/cthread.c ../src/clist.c ../src/log.c ../src/spinlock.c ../src/jmalloc.c -I$(INC) $(OPTS) -lpthread 

dict_test: ../src/dict.c ../src/cstr.c ../src/jmalloc.c
	$(CC) -o $@ dict_test.c $^ -I$(INC)  $(OPTS)

db_test:../src/dict.c ../src/cstr.c ../src/db.c ../src/spinlock.c ../src/obj.c ../src/jmalloc.c
	$(CC) -o $@ db_test.c  $^ -I$(INC)  $(OPTS)

common_test: ../src/common.c
	$(CC) -o $@ common_test.c $^  -I$(INC)  $(OPTS)

timer_test: ../src/jmalloc.c ../src/log.c ../src/ctimer.c ../src/spinlock.c ../src/clist.c ../src/common.c
	$(CC) -o $@ timer_test.c -lpthread -DTHREAD_TEST $^ -I$(INC) $(OPTS)

clean: 
	rm -rf *.dSYM $(TARGS)

