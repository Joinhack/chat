#ifndef DB_H
#define DB_H

#include "dict.h"
#include "obj.h"
#include "lock.h"

typedef struct db {
	dict **tables;
	LOCK_T *locks;
	int table_size;
} db;

db* db_create(size_t s);

void db_destroy(db* db);

int db_set(db *db, size_t tabidx, cstr k, obj *v);

obj* db_get(db *db, size_t tabidx, cstr k);

int db_remove(db *db, size_t tabidx, cstr k);

#endif /**end db define*/
