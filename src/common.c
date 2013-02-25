#include <stdlib.h>
#include <sys/time.h>
#include "common.h"

void time_now(long *s, int *ms) {
	struct timeval now;
	gettimeofday(&now, NULL);
	*s = now.tv_sec;
	*ms = now.tv_usec/1000;
}
