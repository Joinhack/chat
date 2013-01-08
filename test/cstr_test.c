#include <stdio.h>
#include <stdint.h>
#include <cstr.h>

int main(int argc, char const *argv[]) {
	cstr s = create_cstr(1024);
	printf("%d \n", cstr_len(s));
	destory_cstr(s);
	return 0;
}
