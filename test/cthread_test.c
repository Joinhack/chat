#include <stdio.h>
#include <pthread.h>
#include <cthread.h>


void *print_test(void *data) {
	printf("----\n");
	printf("%s\n", (char*)data);
}

int main(int argc, char const *argv[]) {
	int i, ret;
	cthr_pool *pool = create_cthr_pool(10);
	while(1) {
	for(i = 0; i < pool->size; i++) {
		cthread *thr = pool->thrs + i;
		pool->state = THRP_EXIT;
		pthread_cond_broadcast(&thr->cond);
	}
	}

	sleep(1);
	// for(i = 0; i < 10; i++)
	// ret = cthr_pool_run_task(pool, print_test, "eee");
	// sleep(1);
	// printf("=================================\n");
	// for(i = 0; i < 10; i++)
	// ret = cthr_pool_run_task(pool, print_test, "eee");
	// sleep(1);
	//destroy_cthr_pool(pool);
	sleep(5);
	return 0;
}
