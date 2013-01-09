#ifndef CTHREAD_H
#define CTHREAD_H

#include <pthread.h>
#include "cqueue.h"

#define THR_INIT 0x0
#define THR_IDLE 0x1
#define THR_BUSY 0x1<<1
#define THR_EXIT 0x1<<2
#define THRP_WORK 0x0
#define THRP_EXIT 0x1

struct _thr_pool;

typedef void *cthread_proc(void *);

typedef struct {
	pthread_t thrid;
	pthread_cond_t	cond;
	volatile int state;
	cthread_proc *proc;
	void *proc_data;
	struct _thr_pool *pool;
} cthread;

typedef struct _thr_pool {
	cqueue *idle_queue;
	cthread *thrs;
	pthread_mutex_t mutex;
	volatile int state;
	size_t size;
} cthr_pool;

cthr_pool *create_cthr_pool(size_t size);
void destroy_cthr_pool(cthr_pool *pool);
int cthr_pool_run_task(cthr_pool *pool, cthread_proc *proc, void *data);

#endif /*CTHREAD_H*/
