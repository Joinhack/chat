#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include "log.h"
#include "lock.h"
#include "cio.h"

static char* level_array[] = {
	"INFO",
	"WARN",
	"ERR"
};

static int logfd;

void set_logfd(int fd) {
	logfd = fd;
}

static void now(char *buf, size_t len) {
	struct timeval now;
	time_t current_tv;
	char tbuf[128];
	memset(tbuf, 0, sizeof(tbuf));
	gettimeofday(&now, NULL);
	current_tv = now.tv_sec;
	strftime(tbuf, sizeof(tbuf),"%m-%d  %H:%M:%S.",localtime(&current_tv));
	strcat(tbuf, "%ld");
	snprintf(buf, len, tbuf, now.tv_usec/1000);
}

static void log_fmt(char *buf, size_t len, int level, const char *fmt, va_list arg_list) {
	char inner_fmt[65535], nowstr[128];
	memset(inner_fmt, 0, sizeof(inner_fmt));
	memset(nowstr, 0, sizeof(nowstr));
	now(nowstr, sizeof(nowstr));
	snprintf(inner_fmt, sizeof(inner_fmt),"%s[%s]:%s", nowstr, level_array[level], fmt);
	vsnprintf(buf, len, inner_fmt, arg_list);
}

void clog(int level, const char *fmt, ...) {
	char buf[65535];
	memset(buf, 0, sizeof(buf));
	va_list arg_list;
	va_start(arg_list, fmt);
	log_fmt(buf, sizeof(buf), level, fmt, arg_list);
	va_end(arg_list);
	cio_write(logfd, buf, sizeof(buf));
}

