#ifndef CSTR_H
#define CSTR_H

#include "common.h"

#define cstr char *

cstr cstr_create(size_t len);
void cstr_destroy(cstr s);
cstr cstr_ncat(cstr s, char *b, size_t l);
void cstr_clear(cstr s);

typedef struct {
    uint32_t len;
    uint32_t free;
    char buff[];
} cstrhdr;

#define HLEN sizeof(cstrhdr)
#define CSTR_REALPTR(s) ((char*)(s - HLEN))
#define CSTR_HDR(s) ((cstrhdr*)(s - HLEN))
#define cstr_len(s) CSTR_HDR(s)->len
#define cstr_used(s) (CSTR_HDR(s)->len - CSTR_HDR(s)->free)


#endif /*end common str define*/
