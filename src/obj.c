#include <stdlib.h>
#include "jmalloc.h"
#include "cstr.h"
#include "obj.h"


obj* obj_create(int type, void *priv) {
	obj *o = jmalloc(sizeof(struct obj));
	o->type = type;
	o->priv = priv;
	o->ref = 1;
	LOCK_INIT(&o->lock);
	return o;
}

obj* dict_obj_create(dict_opts *opts) {
	dict *d = dict_create(opts);
	return obj_create(OBJ_TYPE_DICT, d);
}

void obj_incr(obj *o) {
	OBJ_LOCK(o);
	o->ref++;
	OBJ_UNLOCK(o);
}

void obj_decr(obj *o) {
	OBJ_LOCK(o);
	o->ref--;
	if(o->ref == 0) {
		switch(o->type) {
		case OBJ_TYPE_DICT:
			dict_destroy((dict*)o->priv);
			break;
		}
		OBJ_UNLOCK(o);
		LOCK_DESTROY(&o->lock);
		jfree(o);
		//must be return, the objecj is already released.
		return;
	}
	OBJ_UNLOCK(o);
}
