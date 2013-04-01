#ifndef DB_H
#define DB_H

#include "dict.h"
#include "lock.h"

typedef struct db {
	dict **tables;
	LOCK_T *locks;
	int table_size;
} db;

db* db_create(size_t s);

void db_destroy(db* db);

int db_set(db *db, size_t tabidx, void *k, void *v);

#endif /**end db define*/
