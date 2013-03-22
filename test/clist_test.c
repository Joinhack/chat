#include <stdio.h>
#include <stdint.h>
#include <clist.h>
#include <pthread.h>
#include <spinlock.h>

#define NUMS 10

uint32_t count;

clist *cl;
spinlock_t lock;

void *cqueue_pop_mt(void *data) {
	size_t i = 0;
	while(1) {
		spinlock_lock(&lock);
		count++;
		fprintf(stdout, "%lu pop %lu\n", (long)pthread_self(), clist_len(cl));
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
	clist_item *cqi;
	for(i = 0; i < 6; i++)
		clist_lpush(cl, NULL);
	cqi = cl->head;

	clist_rpop(cl);
	clist_rpop(cl);
	clist_rpop(cl);
	clist_rpop(cl);
	clist_rpop(cl);
	clist_rpop(cl);

	for(i = 0; i < 6; i++)
		clist_lpush(cl, NULL);
	printf("%lu\n", clist_len(cl));
	clist_destroy(cl);

	cl = clist_create();
	for(i = 0; i < 1000; i++)
		clist_rpush(cl, NULL);
	for(i = 0; i < 1000; i++)
		clist_lpush(cl, NULL);
	printf("%lu\n", clist_len(cl));

	for(i = 0; i < 1000; i++)
		clist_lpop(cl);

	for(i = 0; i < 1000; i++)
		clist_rpop(cl);

	printf("%lu\n", clist_len(cl));

	clist_destroy(cl);

	return 0;
}

