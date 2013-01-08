#ifndef CSTR_H
#define CSTR_H

#include "common.h"

#define cstr char *
#define HLEN 4
#define CSTR_REALPTR(s) ((char*)(s - HLEN))

cstr create_cstr(uint32_t len);
void destory_cstr(cstr s);

CINLINE uint32_t cstr_len(cstr s) {
	return (uint32_t)(*(uint32_t*)CSTR_REALPTR(s));
}

#endif /*CSTR_H*/
