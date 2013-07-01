#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "log.h"
#include "jmalloc.h"
#include "clist.h"

#define REMOVE(item) \
_item_remove(item);\
clist_item_destroy(item)

static clist_item *clist_item_create() {
	clist_item *l_item;
	size_t len = sizeof(struct clist_item);
	l_item = jmalloc(len);
	if(l_item == NULL)
		return NULL;
	l_item->next = l_item;
	l_item->prev = l_item;
	return l_item;
}

static void clist_item_destroy(clist_item *item) {
	jfree(item);
}

clist *clist_create() {
	clist *cl = clist_item_create();
	return cl;
}


void clist_item_remove(clist_item *item) {
	REMOVE(item);
}

void *clist_rpop(clist *cl) {
	void *data;
	clist_item *item;
	if(LIST_EMPTY(cl))
		return NULL;
	item = cl->prev;
	data = item->data;
	REMOVE(item);
	return data;
}

void *clist_lpop(clist *cl) {
	void *data;
	clist_item *item;
	if(LIST_EMPTY(cl))
		return NULL;
	item = cl->next;
	data = item->data;
	REMOVE(item);
	return data;
}

int clist_walk_remove(clist *cl, int (*cb)(void *, void *priv), void *priv) {
	int count = 0, c;
	clist_item *current, *next;
	next = cl->next;
	while(1) {
		current = next;
		DEBUG("%p %p %p\n", cl, current, next);
		if(current == cl || current == NULL) break;
		next = current->next;
		if(!cb(current->data, priv)) {
			REMOVE(current);
			count++;
		}
	}
	return count;
}

clist_item* clist_lpush(clist *cl, void *data) {
	clist_item *item, *hprev;
	item = clist_item_create();
	item->data = data;
	_item_add(item, cl, cl->next);
	return item;
}

clist_item* clist_rpush(clist *cl, void *data) {
	clist_item *item, *hprev;
	item = clist_item_create();
	item->data = data;
	_item_add(item, cl->prev, cl);
	return item;
}

void clist_destroy(clist *cl) {
	while(!LIST_EMPTY(cl)) {
		clist_rpop(cl);
	}
	jfree(cl);
}

void clist_move(clist *sl, clist *dl) {
	dl->next = sl->next;
	dl->next->prev = dl;
	dl->prev = sl->prev;
	dl->prev->next = dl;

	sl->next = sl;
	sl->prev = sl;
}
