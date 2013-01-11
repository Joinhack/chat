#ifndef CSTR_H
#define CSTR_H

#include "common.h"

#define cstr char *
#define HLEN 4
#define CSTR_REALPTR(s) ((char*)(s - HLEN))

cstr cstr_create(size_t len);
void cstr_destroy(cstr s);

CINLINE size_t cstr_len(cstr s) {
	return (size_t)(*(uint32_t*)CSTR_REALPTR(s));
}

#endif /*end common str define*/
