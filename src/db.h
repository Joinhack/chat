#ifndef DB_H
#define DB_H

#include "dict.h"
#include "lock.h"

typedef struct db {
	dict **tables;
	LOCK_T *locks;
	int table_size;
} db;

db db_create(size_t s);

#endif /**end db define*/
