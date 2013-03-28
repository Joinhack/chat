#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include "common.h"

void time_now(long *s, int *ms) {
	struct timeval now;
	gettimeofday(&now, NULL);
	*s = now.tv_sec;
	*ms = now.tv_usec/1000;
}

int str2ll(char *p, size_t len, long long *l) {
	unsigned long long v = 0;
	char *ptr = p, c;
	int negtive = 0, i;
	if(ptr[0] == '-') {
		ptr++;
		negtive = 1;
	}
	while(ptr != p + len) {
		c = ptr[0];
		if(c < '0' || c > '9')
			return -1;
		v *= 10;
		i = ptr[0] - '0';
		if(v > ULLONG_MAX - i)
			return -1;
		v += i;
		ptr++;
	}
	if(negtive) {
		if(v > (unsigned long long)LLONG_MIN)
			return -1;
		*l = -v;
	} else {
		if(v > LLONG_MAX)
			return -1;
		*l = v;
	}
	return 0;
}
