#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "jmalloc.h"
#include "spinlock.h"
#include "ctimer.h"



struct ctimer_base {
	spinlock_t lock;
	volatile uint64_t ctimer_jiffies;
	clist* list;
};

void ctimer_set_jiffies(ctimer_base *b, uint64_t j) {
	b->ctimer_jiffies = j;
}

uint64_t ctimer_get_jiffies(ctimer_base *b) {
	return b->ctimer_jiffies;	
}

static inline void add_ctimer_inner(struct ctimer_base *base, ctimer *ctimer) {
	ctimer->item = clist_rpush(base->list, ctimer);
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
	tb->list = clist_create();
	return tb;
}


static inline int runner_walk_cb(void *data, void *priv) {
	ctimer *ctimer = (struct ctimer*)data;
	ctimer_base *base = (ctimer_base *)priv;
	if(((long)ctimer->expires - (long)base->ctimer_jiffies ) > 0)
		return 1;
	//spinlock_unlock(&base->lock);
	ctimer->item = NULL;
	if(ctimer->cb)
	 	ctimer->cb(ctimer);
	//spinlock_lock(&base->lock);
	return 0;
}

static inline void run_ctimers_inner(ctimer_base *base) {
	clist list;
	spinlock_lock(&base->lock);
	if(!LIST_EMPTY(base->list)) {
		clist_walk_remove(base->list, runner_walk_cb, base);
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
	clist_destroy(tb->list);
	jfree(tb);
}