#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#ifndef CINLINE
#define CINLINE static inline //USE C99 keyword.
#endif

typedef uint64_t msec_t;

#ifdef __linux__
#define USE_EPOLL
#endif

#if (defined(__APPLE__)) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
#define USE_KQUEUE
#endif

void time_now(long *s, int *ms);

#endif /*end define common head*/
