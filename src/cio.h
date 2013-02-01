#ifndef CIO_H
#define CIO_H

#include "cstr.h"

#define IO_TCP 0x1
#define IO_UN 0x1<<1
#define IO_UDP 0x1<<2

typedef struct {
	int fd;
	int type;
	cstr readbuf;
	cstr writebuf;
} cio;

int cio_set_noblock(int fd);

int cio_set_block(int fd);

int cio_write(int fd, char *ptr, size_t len);

int cio_read(int fd, char *ptr, size_t len);

cio *cio_create();

void cio_destroy(cio *io);

#endif /*end define common io**/
