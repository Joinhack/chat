#include <stdio.h>
#include <pthread.h>
#include <cthread.h>


void *print_test(void *data) {
	printf("----\n");
	//printf("%s\n", (char*)data);
}

int main(int argc, char const *argv[]) {
	int i, ret;
	cthr_pool *pool = create_cthr_pool(10);
	for(i = 0; i < 100000000; i++) {
		cthr_pool_run_task(pool, print_test, NULL);
	}

	destroy_cthr_pool(pool);
	return 0;
}