#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "jmalloc.h"
#include "cstr.h"

#define CSH_USED(c) (c->len - c->free)

cstr cstr_create(size_t len) {
	char *c = jmalloc(len + HLEN);
	cstrhdr *csh = (cstrhdr*)c;
	csh->len = len;
	csh->free = len;
	return (cstr)csh->buff;
}

cstr cstr_new(char *c, size_t len) {
	cstr s = cstr_create(len + 1);
	cstrhdr *csh = (cstrhdr*)s;
	memcpy(s, c, len);
	s[len] = '\0';
	csh->free = 0;
	return s;
}

void cstr_destroy(cstr s) {
	char *ptr = CSTR_REALPTR(s);
	jfree(ptr);
}

cstr cstr_extend(cstr s, size_t l) {
	cstrhdr *csh = CSTR_HDR(s);
	if(csh->free >= l) return s;
	csh->len = (csh->len + l)*2;
	csh = jrealloc((void*)csh, csh->len);
	return (cstr)csh->buff;
}

cstr cstr_ncat(cstr s, char *b, size_t l) {
	cstrhdr *csh;
	s = cstr_extend(s, l);
	csh = CSTR_HDR(s);
	memcpy(csh->buff + CSH_USED(csh), b, l);
	csh->free -= l;
	return (cstr)csh->buff;
}

void cstr_clear(cstr s) {
	cstrhdr *csh = CSTR_HDR(s);
	csh->free = csh->len;
	csh->buff[0] = '\0';
}

cstr* cstr_split(char *s, size_t len, char *b, size_t slen, size_t *l) {
	cstr *array = NULL;
	size_t i, j, cap = 0, size = 0, beg = 0;
	for(i = 0; i < len - slen; i++) {
		if(size <= cap) {
			cap += 5;
			array = jrealloc(array, cap);
		}
		if(s[i] == b[0] && memcmp(s + i, b, slen) == 0) {
			array[size] = cstr_new(s + beg, i - beg);
			beg = i + slen; 
			size++;
		}
	}
	*l = size;
	return array;
}
