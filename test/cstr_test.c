#include <stdio.h>
#include <stdint.h>
#include <cstr.h>

int main(int argc, char const *argv[]) {
	cstr s = cstr_create(1024);
	cstr_ncat(s, "asasasas", 8);
	printf("%u %u \n", cstr_len(s), cstr_used(s));
	cstr_ncat(s, "asasasas", 8);
	printf("%s\n", s);
	printf("%u %u \n", cstr_len(s), cstr_used(s));
	cstr_destroy(s);
	return 0;
}
