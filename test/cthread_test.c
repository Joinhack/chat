#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <spinlock.h>
#include <cqueue.h>
#include <cthread.h>


typedef struct {
	cqueue *cq;
	spinlock_t lock;
} data;

void *print_test(void *d) {
	data *datap = (data*)d;
	while(1) {
		spinlock_lock(&datap->lock);
		cqueue_pop(datap->cq);	
		spinlock_unlock(&datap->lock);
		if(cqueue_len(datap->cq) == 0)
			break;
	}
	return NULL;
}

int main(int argc, char const *argv[]) {
	int i, ret;
	data data;
	data.cq = cqueue_create();
	data.lock = SL_UNLOCK;
	cthr_pool *pool = cthr_pool_create(20); 

	for(;;) {
		for(i = 0; i < 10; i++){
			spinlock_lock(&data.lock);
			cqueue_push(data.cq, NULL);
			spinlock_unlock(&data.lock);
		}
		cthr_pool_run_task(pool, print_test, &data);
	}
	cthr_pool_destroy(pool);
	return 0;
}
