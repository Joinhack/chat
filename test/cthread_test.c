#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <spinlock.h>
#include <clist.h>
#include <cthread.h>


typedef struct {
	clist *cq;
	spinlock_t lock;
} data;

void *print_test(void *d) {
	data *datap = (data*)d;
	while(1) {
		spinlock_lock(&datap->lock);
		clist_rpop(datap->cq);	
		spinlock_unlock(&datap->lock);
		if(LIST_EMPTY(datap->cq))
			break;
	}
	return NULL;
}

int main(int argc, char const *argv[]) {
	int m, i, ret;
	data data;
	cthread *thr;
	data.cq = clist_create();
	data.lock = SL_UNLOCK;
	cthr_pool *pool = cthr_pool_create(10); 
	cthr_pool_destroy(pool);
	pool = cthr_pool_create(100); 
	for(m = 0; m < 100; m++) {
		for(i = 0; i < 10; i++){
			spinlock_lock(&data.lock);
			clist_lpush(data.cq, NULL);
			spinlock_unlock(&data.lock);
		}
		cthr_pool_run_task(pool, print_test, &data);
	}
	cthr_pool_destroy(pool);
	for(i = 0; i < pool->size; i++) {
		thr = pool->thrs + i;
		printf("%d\n", thr->state);
	}
	return 0;
}
