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
		if(clist_len(datap->cq) == 0)
			break;
	}
	return NULL;
}

int main(int argc, char const *argv[]) {
	int i, ret;
	data data;
	data.cq = clist_create();
	data.lock = SL_UNLOCK;
	cthr_pool *pool = cthr_pool_create(20); 

	for(;;) {
		for(i = 0; i < 10; i++){
			spinlock_lock(&data.lock);
			clist_lpush(data.cq, NULL);
			spinlock_unlock(&data.lock);
		}
		cthr_pool_run_task(pool, print_test, &data);
	}
	cthr_pool_destroy(pool);
	return 0;
}
