#include <stdio.h>
#include <stdint.h>
#include <cqueue.h>
#include <pthread.h>
#include <spinlock.h>

#define NUMS 10

uint32_t count;

cqueue *cq;
spinlock_t lock;

void *cqueue_pop_mt(void *data) {
	size_t i = 0;
	while(1) {
		spinlock_lock(&lock);
		count++;
		fprintf(stdout, "%ld pop %ld\n", pthread_self(), cqueue_len(cq));
		cqueue_pop(cq);
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
	cq = cqueue_create();
	count = 0;
	cqueue_item *cqi;
	for(i = 0; i < 6; i++)
		cqueue_push(cq, NULL);
	cqi = cq->head;

	cqueue_pop(cq);
	cqueue_pop(cq);
	cqueue_pop(cq);
	cqueue_pop(cq);
	cqueue_pop(cq);
	cqueue_pop(cq);

	for(i = 0; i < 6; i++)
		cqueue_push(cq, NULL);
	cqueue_walk_remove(cq, compare, NULL);
	printf("%d\n", cqueue_len(cq));
	cqueue_destroy(cq);
	return 0;
}

