#include <stdint.h>
#include <stdio.h>
#include <atomic.h>
#include <spinlock.h>
#include <jmalloc.h>
#include <timer.h>


void cb(timer *t) {
	//printf("%lld\n", t->expires);
	t->expires += 1;
	timer_add(t->base, t);
}

int main() {
	timer_base* tb = timer_base_create();
	
	timer *tr[2];
	tr[0] = timer_create();
	tr[1] = timer_create();
	tr[0]->cb = cb;
	timer_add(tb, tr[0]);
	tr[1]->cb = cb;
	timer_add(tb, tr[1]);
	printf("%llu\n", used_mem());
	for(int i = 0; i < 10000000; i++) {
		timer_set_jiffies(tb, i);
		timer_run(tb);
	}

	printf("%llu\n", used_mem());
	timer_base_destroy(tb);
	printf("%llu\n", used_mem());
}