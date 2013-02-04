#include <stdio.h>
#include <stdint.h>
#include <cstr.h>

int main(int argc, char const *argv[]) {
	cstr s = cstr_create(1024), *array;
	size_t len;
	cstr_ncat(s, "asasasas", 8);
	printf("%u %u \n", cstr_len(s), cstr_used(s));
	cstr_ncat(s, "asasasas", 8);
	printf("%s\n", s);
	printf("%u %u \n", cstr_len(s), cstr_used(s));
	array = cstr_split("1212 1212 1212", 14, " ", 1, &len);
	printf("array len %lu\n", len);
	cstr_destroy(s);
	return 0;
}
