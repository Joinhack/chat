#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "jmalloc.h"
#include "spinlock.h"
#include "ctimer.h"


#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)


struct ctimer_base {
	spinlock_t lock;
	volatile uint64_t ctimer_jiffies;
	clist** tv1;
	clist** tv2;
	clist** tv3;
	clist** tv4;
	clist** tv5;
};

void ctimer_set_jiffies(ctimer_base *b, uint64_t j) {
	b->ctimer_jiffies = j;
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

static void add_ctimer_inner(struct ctimer_base *base, ctimer *ctimer) {
	uint64_t expires = ctimer->expires;
	size_t idx = expires - base->ctimer_jiffies;
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
		vec = base->tv1[(base->ctimer_jiffies & TVR_MASK)];
	} else {
		int i;
		if (idx > 0xffffffffUL) {
			idx = 0xffffffffUL;
			expires = idx + base->ctimer_jiffies;
		}
		i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
		vec = base->tv5[i];
	}
	/*
	* ctimers are FIFO:
	*/
	ctimer->item = clist_rpush(vec, ctimer);
}

static void remove_ctimer_inner(ctimer *ctimer) {
	clist_item_remove(ctimer->item);
	ctimer->item = NULL;
}

void ctimer_remove(ctimer *t) {
	if(t->base == NULL)
		return;
	ctimer_base *base = t->base;
	spinlock_lock(&base->lock);
	if(t->item == NULL) {
		spinlock_unlock(&base->lock);
		return;
	}
	remove_ctimer_inner(t);
	spinlock_unlock(&base->lock);
}

void ctimer_add(ctimer_base *base, ctimer *t) {
	spinlock_lock(&base->lock);
	t->base = base;
	add_ctimer_inner(base, t);
	spinlock_unlock(&base->lock);
}

ctimer_base* ctimer_base_create() {
	struct ctimer_base *tb = jmalloc(sizeof(struct ctimer_base));
	tb->lock = SL_UNLOCK;
	tb->tv1 = tv_init(TVR_SIZE);
	tb->tv2 = tv_init(TVN_SIZE);
	tb->tv3 = tv_init(TVN_SIZE);
	tb->tv4 = tv_init(TVN_SIZE);
	tb->tv5 = tv_init(TVN_SIZE);
	return tb;
}

static inline int cascade_walk_cb(void *data, void *priv) {
	ctimer *t = (ctimer*)data;
	ctimer_base *base = (ctimer_base *)priv;
	add_ctimer_inner(base, t);
	return 0;
}

static int cascade(struct ctimer_base *base, clist **tv, int index) {
	clist list;
	if(LIST_EMPTY(tv[index]))
		return index;
	clist_move(tv[index], &list);
	clist_walk_remove(&list, cascade_walk_cb, base);
	return index;
}

#define INDEX(N) ((base->ctimer_jiffies >> (TVR_BITS + (N) * TVN_BITS)) & TVN_MASK)

static inline int runner_walk_cb(void *data, void *priv) {
	ctimer *ctimer = (struct ctimer*)data;
	ctimer_base *base = (ctimer_base *)priv;
	ctimer->item = NULL;
	spinlock_unlock(&base->lock);
	if(ctimer->cb) 
		ctimer->cb(ctimer);
	spinlock_lock(&base->lock);
	return 0;
}

static inline void run_ctimers_inner(ctimer_base *base) {
	clist list;
	spinlock_lock(&base->lock);
	int index = base->ctimer_jiffies & TVR_MASK;
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

void ctimer_run(ctimer_base *base) {
	run_ctimers_inner(base);
}

ctimer* ctimer_create() {
	size_t s = sizeof(ctimer);
	ctimer *t = jmalloc(s);
	memset(t, 0, s);
	return t;
}

void ctimer_destroy(ctimer* t) {
	ctimer_base *base = t->base;
	if(t->item != NULL)
		ctimer_remove(t);
	jfree(t);
}

void ctimer_base_destroy(struct ctimer_base* tb) {
	tb->lock = SL_UNLOCK;
	tv_destroy(tb->tv1, TVR_SIZE);
	tv_destroy(tb->tv2, TVN_SIZE);
	tv_destroy(tb->tv3, TVN_SIZE);
	tv_destroy(tb->tv4, TVN_SIZE);
	tv_destroy(tb->tv5, TVN_SIZE);
	jfree(tb);
}