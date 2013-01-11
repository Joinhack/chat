#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "jmalloc.h"
#include "cio.h"

int cio_set_noblock(int fd) {
	int val;
	val = 1;
	return ioctl(fd, FIONBIO, &val);
}

int cio_set_block(int fd) {
	int val;
	val = 0;
	return ioctl(fd, FIONBIO, &val);
}

int cio_write(int fd, char *ptr, size_t len) {
	int count = 0;
	int wn = 0;
	while(count < len ) {
		wn = write(fd, ptr, len - count);
		if(wn == 0) return count;
		if(wn == -1) return -1;
		ptr += wn;
		count += wn;
	}
	return count;
}

int cio_read(int fd, char *ptr, size_t len) {
	int count = 0;
	int rn = 0;
	while(count < len ) {
		rn = read(fd, ptr, len - count);
		if(rn == 0) return count;
		if(rn == -1) return -1;
		ptr += rn;
		count += rn;
	}
	return count;
}

cio *cio_create() {
	cio *io = jmalloc(sizeof(cio));
	memset(io, 0, sizeof(cio));
	io->buff = cstr_create(1024);
	return io;
}

void cio_destroy(cio *io) {
	cstr_destroy(io->buff);
	jfree(io);
}
