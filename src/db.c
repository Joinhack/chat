#include <stdlib.h>
#include <string.h>
#include "jmalloc.h"
#include "cstr.h"
#include "db.h"
#include "obj.h"

unsigned int hash(const void *okey) {
	cstr key = (cstr)((obj*)okey)->priv;
	return dict_generic_hash((char*)key, strlen((char*)key));
}

void db_value_destroy(void *k) {
	obj *o = (obj*)k;
	obj_decr(o);
}

void db_value_dup(void *k) {
	obj *o = (obj*)k;
	obj_incr(o);
}

void *db_key_dup(const void *k) {
	obj *o = (obj*)k;
	obj_incr(o);
	return o;
}

void db_key_destroy(void *k) {
	obj *o = (obj*)k;
	obj_decr(o);
}

void db_destroy(db* db) {
	size_t i;
	for(i = 0; i < db->table_size; i++) {
		LOCK(&db->locks[i]);
		dict_destroy(db->tables[i]);
		UNLOCK(&db->locks[i]);
		LOCK_DESTROY(&db->locks[i]);
	}
	jfree(db->tables);
	jfree(db->locks);
}

int db_key_compare(const void *k1, const void *k2) {
	size_t len;
	cstr s1 = (cstr)((obj*)k1)->priv;
	cstr s2 = (cstr)((obj*)k2)->priv;
	len = cstr_used(s1);
	if(len != cstr_used(s2))
		return 0;
	return memcmp(s1, s2, len) == 0;
}

dict_opts db_opts = {
	.hash = hash,
	db_key_dup,
	NULL,
	db_key_compare,
	db_key_destroy,
	db_value_destroy,
};

db* db_create(size_t s) {
	size_t i;
	db *db = jmalloc(s);
	db->tables = jmalloc(sizeof(struct dict*)*s);
	db->locks = jmalloc(sizeof(LOCK_T)*s);
	for(i = 0; i < s; i++) {
		db->tables[i] = dict_create(&db_opts);
		LOCK_INIT(&db->locks[i]);
	}
	db->table_size = s;
	return db;
}

int db_set(db *db, size_t tabidx, obj *k, obj *v) {
	dict *d;
	int rs;
	d = db->tables[tabidx];
	LOCK(&db->locks[tabidx]);
	dict_replace(d, k, v);
	UNLOCK(&db->locks[tabidx]);
}

int db_remove(db *db, size_t tabidx, obj *k) {
	dict *d;
	int rs;
	d = db->tables[tabidx];
	LOCK(&db->locks[tabidx]);
	rs = dict_del(d, k);
	UNLOCK(&db->locks[tabidx]);
	return rs;
}

obj* db_get(db *db, size_t tabidx, obj *k) {
	dict *d;
	int rs;
	obj *o = NULL;
	dict_entry *entry;
	d = db->tables[tabidx];
	LOCK(&db->locks[tabidx]);
	entry = dict_find(d, k);
	if(entry != NULL)
		o = (obj*)entry->value;
	UNLOCK(&db->locks[tabidx]);
	if(o) obj_incr(o);
	return o;
}

