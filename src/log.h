#ifndef LOG_H
#define LOG_H

enum LOG_LEVEL {
	LEVEL_INFO=0,
	LEVEL_WARN,
	LEVEL_ERR
};

void log_init(int fd);

void log_print(int level, char *fmt, ...);

#define ERROR(fmt, ...) log_print(LEVEL_ERR, fmt, ##__VA_ARGS__)

#define INFO(fmt, ...) log_print(LEVEL_INFO, fmt, ##__VA_ARGS__)

#define WARN(fmt, ...) log_print(LEVEL_WARN, fmt, ##__VA_ARGS__)

#endif /**LOG_H*/
