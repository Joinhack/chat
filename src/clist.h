#ifndef CLIST_H
#define CLIST_H

#include "common.h"

typedef struct clist_item {
	struct clist_item *prev;
	struct clist_item *next;
	void *data;
} clist_item;

typedef clist_item clist;

static inline void _item_remove(clist_item *item) {
	item->prev->next = item->next;
	item->next->prev = item->prev;
}

static inline void _item_add(clist_item *n, clist_item *prev, clist_item *next) {
	next->prev = n;
	n->next = next;
	n->prev = prev;
	prev->next = n;
}

#define LIST_EMPTY(l) (l->next == l)

#ifdef __cplusplus
extern "C" {
#endif

clist *clist_create();
void clist_destroy(clist *cl);
void *clist_rpop(clist *cl);
clist_item* clist_lpush(clist *cl, void *data);
void *clist_lpop(clist *cl);
clist_item* clist_rpush(clist *cl, void *data);
void clist_item_remove(clist_item *item);
void clist_move(clist *sl, clist *dl);

//return removed count.
int clist_walk_remove(clist *cl, int (*cb)(void *, void *priv), void *priv);

#ifdef __cplusplus
}
#endif

#endif /* end define common list **/
