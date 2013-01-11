#include <stdint.h>
#include <string.h>
#include "jmalloc.h"
#include "cqueue.h"

static cqueue_item *cqueue_item_create() {
	cqueue_item *cq_item;
	size_t len = sizeof(cqueue_item);
	cq_item = jmalloc(len);
	if(cq_item == NULL)
		return NULL;
	cq_item->next = NULL;
	cq_item->prev = NULL;
	cq_item->data = NULL;
	return cq_item;
}

static void cqueue_item_destroy(cqueue_item *item) {
	jfree(item);
}

cqueue *cqueue_create() {
	cqueue *cq;
	size_t len = sizeof(cqueue);
	cq = jmalloc(len);
	cq->head = NULL;
	cq->count = 0;
	return cq;
}

void *cqueue_pop(cqueue *cq) {
	void *data;
	cqueue_item *item;
	if(cq->count == 0) {
		return NULL;
	}
	item = cq->head->prev;
	data = item->data;
	if(--cq->count) {
		item->prev->next = item->next;
		item->next->prev = item->prev;
	} else {
		cq->head = NULL;
	}
	cqueue_item_destroy(item);
	return data;
}

void cqueue_push(cqueue *cq, void *data) {
	cqueue_item *item, *hprev;
	item = cqueue_item_create();
	item->data = data;
	if(cq->head != NULL) {
		hprev = cq->head->prev;
		item->next = cq->head;
		item->prev = cq->head->prev;
		cq->head->prev = item;
		hprev->next = item;
	} else {
		item->prev = item;
		item->next = item;
	}
	cq->head = item;
	cq->count++;
}

void cqueue_destroy(cqueue *cq) {
	while(cq->count > 0) {
		cqueue_pop(cq);
	}
	jfree(cq);
}
