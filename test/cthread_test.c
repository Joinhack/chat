#include <stdio.h>
#include <pthread.h>
#include <cthread.h>


void *print_test(void *data) {
	printf("----\n");
	printf("%s\n", (char*)data);
}

int main(int argc, char const *argv[]) {
	int i, ret;
	cthread_pool *pool = create_cthread_pool(1);
	for(i = 0; i < 10; i++) {
		ret = cthread_pool_submit_task(pool, print_test, "eee");
	}
	sleep(1);
	destory_cthread_pool(pool);
	return 0;
}