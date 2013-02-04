#include <stdio.h>
#include <stdint.h>
#include <cstr.h>

int main(int argc, char const *argv[]) {
	cstr s = cstr_create(1024);
	cstr *array;
	size_t len, i;
	cstr_ncat(s, "asasasas", 8);
	printf("%u %u \n", cstr_len(s), cstr_used(s));
	cstr_ncat(s, "asasasas", 8);
	printf("%s\n", s);
	printf("%u %u \n", cstr_len(s), cstr_used(s));
	array = cstr_split("1 2 3 4 5 ", 11, " ", 1, &len);
	printf("array len %lu\n", len);
	for(i = 0; i < len; i++) {
		printf("array[%lu]:%s\n", i, array[i]);
		cstr_destroy(array[i]);
	}
	jfree(array);
	cstr_destroy(s);
	return 0;
}
