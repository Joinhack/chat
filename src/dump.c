#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "log.h"
#include "server.h"
#include "dump.h"

#define TYPE_TABLE 1
#define TYPE_OBJ 2
#define TYPE_CSTR 3
#define TYPE_END 255

int dump_type(int fd, unsigned char t) {
	return write(fd, &t, 1);
}

int dump_len(int fd, uint32_t l) {
	l = htonl(l);
	return write(fd, &l, 4);
}

int dump_cstr(int fd, cstr s) {
	int count = 0;
	int wn = 0;
	char *ptr = s;
	size_t used = cstr_used(s);
	dump_len(fd, used);
	while(count < used) {
		wn = write(fd, ptr, used - count);
		if(wn == 0) return count;
		if(wn == -1) return -1;
		ptr += wn;
		count += wn;
	}
	return count;
}

int dump_obj(int fd, obj *o) {
	if(o->type == OBJ_TYPE_STR) {
		return dump_cstr(fd, (cstr)o->priv);
	}
	return -1;
}

int dump_obj_type(int fd, obj *o) {
	if(o->type == OBJ_TYPE_STR) {
		return dump_type(fd, TYPE_CSTR);
	}
}

int dump_save(server *svr) {
	size_t i = 0;
	int fd = 0;
	obj *o, *k;
	db *db = svr->db;
	dict_iterator *iter;
	dict_entry *entry;
	char buf[1024] = {0};
	snprintf(buf, sizeof(buf), "tmp-%d.xdb", (int)getpid());
	if((fd = open(buf, O_APPEND|O_CREAT|O_WRONLY)) < 0) goto err;
	if(write(fd, "XDB", 3) < 0) goto err;
	for(i = 0; i < db->table_size; i++) {
		dump_type(fd, TYPE_TABLE);
		dump_len(fd, i);
		iter = dict_get_iterator(db->tables[i]);
		do {
			LOCK(&db->locks[i]);
			entry = dict_iterator_next(iter);
			if(entry == NULL) {
				UNLOCK(&db->locks[i]);
				break;
			}
			o = (obj*)entry->value;
			k = (obj*)entry->key;
			obj_incr(o);
			obj_incr(k);
			UNLOCK(&db->locks[i]);
			if(dump_obj_type(fd, o) < 0) goto err;
			if(dump_obj(fd, k) < 0) goto err;
			if(dump_obj(fd, o) < 0) goto err;
			obj_decr(o);
			obj_decr(k);
		} while(entry != NULL);
		dict_iterator_destroy(iter);
		dump_type(fd, TYPE_END);
	}
	if(rename(buf, "dump.xdb") < 0) goto err;
	chmod("dump.xdb", S_IRUSR|S_IWUSR);
	close(fd);
	return 0;
err:
	ERROR("dump db error: %s\n", strerror(errno));
	if(fd > 0)
		close(fd);
	return -1;
}

int load_type(int fd) {
	unsigned char type;
	if(read(fd, &type, 1) < 0) {
		return -1;
	}
	return type;
}

uint32_t load_len(int fd) {
	uint32_t len = -1;
	if(read(fd, &len, 4) < 0)
		return -1;
	return ntohl(len);
}

cstr load_cstr(int fd) {
	int len = load_len(fd);
	int count = 0;
	int rn = 0;
	char *ptr;
	cstr s;
	if(len < 0) return NULL;
	s = cstr_create(len);
	ptr = s;
	while(count < len) {
		rn = read(fd, ptr, len - count);
		if(rn == 0 || rn == -1) {
			cstr_destroy(s);
			return NULL;
		}
		ptr += rn;
		count += rn;
	}
	return s;
}

obj* load_obj(int fd, int type) {
	obj* o;
	if(type == TYPE_CSTR) {
		cstr v;
		v = load_cstr(fd);
		if(v == NULL)
			return NULL;
		return obj_create(OBJ_TYPE_STR, v);
	}
	return NULL;
}

int dump_load(server *svr) {
	int fd = 0, tabidx;
	char buf[3] = {0};
	db *db = svr->db;
	if((fd = open("dump.xdb", O_RDONLY)) < 0) goto err;
	if(read(fd, buf, 3) < 0) goto err;
	if(memcmp(buf, "XDB", 3) != 0) {
		ERROR("wrong db type\n");
		close(fd);
		return -1;
	}
	while(1) {
		int type;
		int len;
		obj *o, *key;
		if((type = load_type(fd)) < 0) goto err;
		if(type == TYPE_END) break;
		if(type == TYPE_TABLE) {
			tabidx = load_len(fd);
			if(tabidx >= svr->db->table_size) {
				close(fd);
				ERROR("the table index is bigger, load error\n");
				return -1;
			}
		}
		key = load_obj(fd, TYPE_CSTR);
		o = load_obj(fd, type);
		if(key == NULL) goto err;
		if(o == NULL) {
			obj_decr(key);
			goto err;
		}
		db_set(db, tabidx, key, o);
	}
	close(fd);
	return 0;
err:
	ERROR("load db error: %s\n", strerror(errno));
	if(fd > 0)
		close(fd);
	return -1;
}

