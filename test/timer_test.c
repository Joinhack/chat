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
	timer *tr;
	for(int i = 0; i < 10000; i++) {
		tr = timer_create(tb);
		tr->expires = i;
		tr->cb = cb;
		timer_add(tb, tr);
	}
	printf("%llu\n", used_mem());
	for(int i = 0; i < 10000; i++) {
		timer_set_jiffies(tb, i);
		timer_run(tb);
	}
	
	printf("%llu\n", used_mem());
	timer_base_destroy(tb);
	printf("%llu\n", used_mem());
}