#ifndef CTIMER_H
#define CTIMER_H

#include "clist.h"

typedef struct ctimer_base ctimer_base;

typedef struct ctimer {
	uint64_t expires;
	clist_item *item;
	void (*cb)(struct ctimer *ctimer);
	ctimer_base *base;
	void *priv;
} ctimer;

#ifdef __cplusplus
extern "C" {
#endif

ctimer_base* ctimer_base_create();

void ctimer_base_destroy(ctimer_base* tb);

void ctimer_remove(ctimer *t);

void ctimer_add(ctimer_base *base, ctimer *t);

ctimer* ctimer_create();

void ctimer_destroy(ctimer* t);

void ctimer_run(ctimer_base *base);

void ctimer_set_jiffies(ctimer_base *b, uint64_t j);

#ifdef __cplusplus
}
#endif

#endif
