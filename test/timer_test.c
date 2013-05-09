#include <stdint.h>
#include <stdio.h>
#include <atomic.h>
#include <spinlock.h>
#include <jmalloc.h>
#include <timer.h>


void cb(timer *t) {
	printf("%lld\n", t->expires);
	timer_destroy(t);
}

int main() {
	timer_base* tb = timer_base_create();
	printf("%llu\n", used_mem());
	timer *tr[2];
	tr[0] = timer_create();
	tr[1] = timer_create();
	timer_add(tb, tr[0]);
	timer_add(tb, tr[1]);
	timer_remove(tr[0]);
	timer_add(tb, tr[0]);

	printf("%llu\n", used_mem());
	timer_base_destroy(tb);
	printf("%llu\n", used_mem());
}