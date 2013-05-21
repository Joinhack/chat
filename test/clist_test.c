#include <stdio.h>
#include <stdint.h>
#include <clist.h>
#include <pthread.h>
#include <spinlock.h>
#include <jmalloc.h>

#define NUMS 10

uint32_t count;

clist *cl;
spinlock_t lock;

void *cqueue_pop_mt(void *data) {
	size_t i = 0;
	while(1) {
		spinlock_lock(&lock);
		count++;
		fprintf(stdout, "%lu pop\n", (long)pthread_self());
		clist_rpop(cl);
		spinlock_unlock(&lock);
	}
	
	return NULL;
}

int compare(void *data, void *priv) {
	return 1;
}

int main(int argc, char const *argv[]) {
	size_t i;
	lock = SL_UNLOCK;
	cl = clist_create();
	count = 0;
	for(i = 0; i < 6; i++)
		clist_lpush(cl, NULL);

	clist_rpop(cl);
	clist_rpop(cl);
	clist_rpop(cl);
	clist_rpop(cl);
	clist_rpop(cl);
	clist_rpop(cl);

	for(i = 0; i < 6; i++)
		clist_lpush(cl, NULL);
	clist_destroy(cl);

	cl = clist_create();
	for(i = 0; i < 1000; i++)
		clist_rpush(cl, NULL);
	for(i = 0; i < 1000; i++)
		clist_lpush(cl, NULL);

	for(i = 0; i < 1000; i++)
		clist_lpop(cl);

	for(i = 0; i < 1000; i++)
		clist_rpop(cl);

	clist_destroy(cl);
	printf("%llu\n", used_mem());

	return 0;
}

