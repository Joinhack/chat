#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "jmalloc.h"
#include "spinlock.h"
#include "timer.h"


#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)


struct timer_base {
	spinlock_t lock;
	volatile uint64_t timer_jiffies;
	clist** tv1;
	clist** tv2;
	clist** tv3;
	clist** tv4;
	clist** tv5;
};

void timer_set_jiffies(timer_base *b, uint64_t j) {
	b->timer_jiffies = j;
}

static clist** tv_init(size_t s) {
	size_t i;
	clist** tv = jmalloc(sizeof(clist*)*s);
	for(i = 0; i < s; i++) {
		tv[i] = clist_create();
	}
	return tv;
}

static void tv_destroy(clist** l,size_t s) {
	size_t i;
	for(i = 0; i < s; i++) {
		clist_destroy(l[i]);
	}
	jfree(l);
}

static void add_timer_inner(struct timer_base *base, timer *timer) {
	uint64_t expires = timer->expires;
	size_t idx = expires - base->timer_jiffies;
	clist *vec;
	if (idx < TVR_SIZE) {
		int i = expires & TVR_MASK;
		vec = base->tv1[i];
	} else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
		int i = (expires >> TVR_BITS) & TVN_MASK;
		vec = base->tv2[i];
	} else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
		vec = base->tv3[i];
	} else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
		vec = base->tv4[i];
	} else if ((int64_t) idx < 0) {
		vec = base->tv1[(base->timer_jiffies & TVR_MASK)];
	} else {
		int i;
		if (idx > 0xffffffffUL) {
			idx = 0xffffffffUL;
			expires = idx + base->timer_jiffies;
		}
		i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
		vec = base->tv5[i];
	}
	/*
	* Timers are FIFO:
	*/
	timer->item = clist_rpush(vec, timer);
	timer->vec = vec;
}

static void remove_timer_inner(timer *timer) {
	clist_item_remove(timer->item);
	timer->item = NULL;
	timer->vec = NULL;
}

void timer_remove(timer *t) {
	timer_base *base = t->base;
	spinlock_lock(&base->lock);
	if(t->item == NULL || t->vec == NULL) {
		spinlock_unlock(&base->lock);
		return;
	}
	remove_timer_inner(t);
	spinlock_unlock(&base->lock);
}

void timer_add(timer_base *base, timer *t) {
	spinlock_lock(&base->lock);
	t->base = base;
	add_timer_inner(base, t);
	spinlock_unlock(&base->lock);
}

timer_base* timer_base_create() {
	struct timer_base *tb = jmalloc(sizeof(struct timer_base));
	tb->lock = SL_UNLOCK;
	tb->tv1 = tv_init(TVR_SIZE);
	tb->tv2 = tv_init(TVN_SIZE);
	tb->tv3 = tv_init(TVN_SIZE);
	tb->tv4 = tv_init(TVN_SIZE);
	tb->tv5 = tv_init(TVN_SIZE);
	return tb;
}

static inline int cascade_walk_cb(void *data, void *priv) {
	timer *t = (timer*)data;
	timer_base *base = (timer_base *)priv;
	add_timer_inner(base, t);
	return 0;
}

static int cascade(struct timer_base *base, clist **tv, int index) {
	clist list;
	if(LIST_EMPTY(tv[index]))
		return index;
	clist_move(tv[index], &list);
	clist_walk_remove(&list, cascade_walk_cb, base);
	return index;
}

#define INDEX(N) ((base->timer_jiffies >> (TVR_BITS + (N) * TVN_BITS)) & TVN_MASK)

static inline int runner_walk_cb(void *data, void *priv) {
	timer *timer = (struct timer*)data;
	timer_base *base = (timer_base *)priv;
	timer->item = NULL;
	timer->vec = NULL;
	spinlock_unlock(&base->lock);
	if(timer->cb) 
		timer->cb(timer);
	spinlock_lock(&base->lock);
	return 0;
}

static inline void run_timers_inner(timer_base *base) {
	clist list;
	spinlock_lock(&base->lock);
	int index = base->timer_jiffies & TVR_MASK;
	if (!index &&
			(!cascade(base, base->tv2, INDEX(0))) &&
				(!cascade(base, base->tv3, INDEX(1))) &&
					!cascade(base, base->tv4, INDEX(2)))
						cascade(base, base->tv5, INDEX(3));
	if(!LIST_EMPTY(base->tv1[index])) {
		clist_move(base->tv1[index], &list);
		clist_walk_remove(&list, runner_walk_cb, base);
	}
	spinlock_unlock(&base->lock);
}

void timer_run(timer_base *base) {
	run_timers_inner(base);
}

timer* timer_create() {
	size_t s = sizeof(timer);
	timer *t = jmalloc(s);
	memset(t, 0, s);
	return t;
}

void timer_destroy(timer* t) {
	timer_base *base = t->base;
	if(t->item != NULL && t->vec != NULL)
		timer_remove(t);
	jfree(t);
}

void timer_base_destroy(struct timer_base* tb) {
	tb->lock = SL_UNLOCK;
	tv_destroy(tb->tv1, TVR_SIZE);
	tv_destroy(tb->tv2, TVN_SIZE);
	tv_destroy(tb->tv3, TVN_SIZE);
	tv_destroy(tb->tv4, TVN_SIZE);
	tv_destroy(tb->tv5, TVN_SIZE);
	jfree(tb);
}