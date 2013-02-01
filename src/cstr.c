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

cstr* cstr_split(cstr s, char *b, size_t *l) {
	
}
