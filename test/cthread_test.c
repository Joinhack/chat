#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <cthread.h>


void *print_test(void *data) {
	printf("----\n");
	//printf("%s\n", (char*)data);
	return NULL;
}

int main(int argc, char const *argv[]) {
	int i, ret;
	cthr_pool *pool = cthr_pool_create(10);
	for(i = 0; i < 1000000; i++) {
		cthr_pool_run_task(pool, print_test, NULL);
	}
	cthr_pool_destroy(pool);
	return 0;
}
