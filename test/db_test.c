#include <stdio.h>
#include <cstr.h>
#include <db.h>

int main() {
	size_t i;
	obj *op;
	db* db = db_create(16);
	cstr k = cstr_new("1", 1);	
	obj* obj = cstr_obj_create("2");
	db_set(db, 0, k, obj);
	for(i = 0; i < 1; i++) {
		op = db_get(db, 0, k);
		obj_decr(op);
		op = db_get(db, 0, k);
		obj_decr(op);
		op = db_get(db, 0, k);
		obj_decr(op);
	}
	return 0;
}