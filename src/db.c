#include <stdlib.h>
#include <string.h>
#include "jmalloc.h"
#include "cstr.h"
#include "db.h"
#include "obj.h"

unsigned int hash(const void *key) {
	return dict_generic_hash((char*)key, strlen((char*)key));
}

void db_value_destroy(void *k) {
	obj *o = (obj*)k;
	obj_decr(o);
}

void db_key_destroy(void *k) {
	cstr_destroy((cstr)k);
}

int db_key_compare(const void *k1, const void *k2) {
	size_t len;
	cstr s1 = (cstr)k1;
	cstr s2 = (cstr)k2;
	len = cstr_len(s1);
	if(len != cstr_len(s2))
		return 1;
	return memcmp(k1, k2, len);
}

dict_opts db_opts = {
	.hash = hash,
	NULL,
	NULL,
	db_key_compare,
	db_key_destroy,
	db_value_destroy,
};

db db_create(size_t s) {
	size_t i;
	db *db = jmalloc(s);
	db->tables = jmalloc(sizeof(struct dict*)*s);
	for(i = 0; i < s; i++) {
		db->tables[i] = dict_create(&db_opts);
	}
	db->table_size = s;
}

