#include <stdint.h>
#include <stdio.h>
#include <atomic.h>
#include <spinlock.h>
#include <jmalloc.h>
#include <ctimer.h>


void cb(ctimer *t) {
	//printf("%lld\n", t->expires);
	t->expires += 1;
	ctimer_remove(t);
	ctimer_add(t->base, t);
}

int main() {
	ctimer_base* tb = ctimer_base_create();
	
	ctimer *tr[2];
	tr[0] = ctimer_create();
	tr[1] = ctimer_create();
	tr[0]->cb = cb;
	ctimer_add(tb, tr[0]);
	tr[1]->cb = cb;
	ctimer_add(tb, tr[1]);
	printf("%llu\n", used_mem());
	for(int i = 0; i < 1000000; i++) {
		ctimer_set_jiffies(tb, i);
		ctimer_run(tb);
	}

	ctimer_destroy(tr[0]);
	ctimer_destroy(tr[1]);
	printf("%llu\n", used_mem());
	ctimer_base_destroy(tb);
	printf("%llu\n", used_mem());
}