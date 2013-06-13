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

#ifdef __cplusplus
extern "C" {
#endif

db* db_create(int s);

void db_destroy(db* db);

int db_set(db *db, size_t tabidx, obj *k, obj *v);

obj* db_get(db *db, size_t tabidx, obj *k);

int db_remove(db *db, size_t tabidx, obj *k);

#ifdef __cplusplus
}
#endif

#endif /**end db define*/
