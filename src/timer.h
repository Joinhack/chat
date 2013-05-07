#ifndef TIMER_H
#define TIMER_H

#include "clist.h"

typedef struct timer_base timer_base;

typedef struct timer{
	uint64_t expires;
	clist_item *item;
	clist *vec;
	void (*cb)(struct timer *timer);
	timer_base *base;
} timer;

timer_base* timer_base_create();

void timer_base_destroy(timer_base* tb);

void timer_remove(timer *t);

void timer_add(timer_base *base, timer *t);

timer* timer_create(timer_base *base);

void timer_destroy(timer* t);

void timer_run(timer_base *base);

void timer_set_jiffies(timer_base *b, uint64_t j);

#endif
