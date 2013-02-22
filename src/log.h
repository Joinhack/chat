#ifndef LOG_H
#define LOG_H

enum LOG_LEVEL { 
	LEVEL_INFO = 0,
	LEVEL_WARN,
	LEVEL_ERR
};

void log_init(int fd);

void clog(int level, const char *fmt, ...);

#define ERROR(fmt, ...) clog(LEVEL_ERR, fmt, ##__VA_ARGS__)

#define INFO(fmt, ...) clog(LEVEL_INFO, fmt, ##__VA_ARGS__)

#define WARN(fmt, ...) clog(LEVEL_WARN, fmt, ##__VA_ARGS__)

#endif /**LOG_H*/
