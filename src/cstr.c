#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "jmalloc.h"
#include "cstr.h"

cstr create_cstr(uint32_t len) {
	char *c = jmalloc(len + HLEN);
	(*(uint32_t*)c) = len;
	return (cstr)(c + HLEN);
}

void destory_cstr(cstr s) {
	char *ptr = CSTR_REALPTR(s);
	jfree(ptr);
}

