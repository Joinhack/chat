#ifndef CQUEUE_H
#define CQUEUE_H

#include "common.h"

typedef struct _cqueue_item {
	struct _cqueue_item *prev;
	struct _cqueue_item *next;
	void *data;
} cqueue_item;

typedef struct {
	cqueue_item *head;
	size_t count;
} cqueue;

cqueue *cqueue_create();
void cqueue_destroy(cqueue *cq);
void *cqueue_pop(cqueue *cq);
void cqueue_push(cqueue *cq, void *data);

//return removed count.
int cqueue_walk_remove(cqueue *cq, int (*cb)(void *, void *priv), void *priv);

CINLINE size_t cqueue_len(cqueue *cq) {
	return cq->count;
}

#endif /* end define common queue **/
