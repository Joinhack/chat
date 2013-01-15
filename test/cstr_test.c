#include <stdio.h>
#include <stdint.h>
#include <cstr.h>

int main(int argc, char const *argv[]) {
	cstr s = cstr_create(1024);
	printf("%ld \n", cstr_len(s));
	cstr_destroy(s);
	return 0;
}
