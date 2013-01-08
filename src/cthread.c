#include <stdio.h>
#include "jmalloc.h"
#include "cthread.h"

static cthread *create_cthread(cthread_pool *p);
static void destory_cthread(cthread  *thr);

void *thread_loop(void *data) {
	cthread *thr = (cthread*)data;
	cthread_pool *pool = thr->pool;
	printf("%s\n", "wait");
	for(;;) {
		thr->state = THR_IDLE;
		spinlock_lock(&pool->idle_lock);
		cqueue_push(pool->idle_queue, (void*)thr);
		spinlock_unlock(&pool->idle_lock);
		if(pthread_cond_wait(&thr->cond, &pool->mutex) < 0) {
			//TODO: log it.
			fprintf(stderr, "wait error\n");
			return NULL;
		}
		if(pool->state == THRP_EXIT) return NULL;
		thr->state = THR_BUSY;
		if(thr->proc)
			thr->proc(thr->proc_data);
		thr->proc = NULL;
		thr->proc_data = NULL;
	}
}

static int cthread_init(cthread *thr, cthread_pool *pool) {
	pthread_attr_t  thr_attr;
	thr->pool = pool;
	if(pthread_cond_init(&thr->cond, NULL) < 0) {
		fprintf(stderr, "create thread error\n");
		return -1;
	}
	if(pthread_create(&thr->thrid, &thr_attr, thread_loop, thr) < 0) {
		//TODO: log it.
		fprintf(stderr, "create thread error\n");
		return -1;
	}
	printf("---%d\n", thr->thrid);
	return 0;
}

void destory_cthread_pool(cthread_pool *pool) {
	cthread *thr;
	void *thr_ret;
	int i;
	pool->state = THRP_EXIT;
	for(i = 0; i < pool->size; i++) {
		thr = pool->thrs + i;
		pthread_join(thr->thrid, &thr_ret);
	}
	jfree(pool->thrs);
	jfree(pool);
}

cthread_pool *create_cthread_pool(size_t size) {
	int i, ret;
	cthread *thr;
	cthread_pool *pool = (cthread_pool *)jmalloc(sizeof(cthread_pool));
	pool->idle_lock = SL_UNLOCK;
	pool->thrs = jmalloc(sizeof(cthread)*size);
	ret = pthread_mutex_init(&pool->mutex, NULL);
	if(ret < 0) {
		//TODO: log it.
		fprintf(stderr, "create thread pool error\n");
		jfree(pool);
		return NULL;
	}
	pool->size = 0;
	pool->idle_queue = create_cqueue();
	pool->state = THRP_WORK;
	for(i = 0; i <  size; i++) {
		thr = pool->thrs + i;
		if(cthread_init(thr, pool) < 0) {
			destory_cthread_pool(pool);
			return NULL;
		}
		pool->size++;
	}
	pool->state = THRP_WORK;
	return pool;
}

//return -1, all thread is busy
int cthread_pool_submit_task(cthread_pool *pool, cthread_proc *proc, void *proc_data) {
	cthread *thr;
	if(cqueue_len(pool->idle_queue) == 0)
		return -1;
	spinlock_lock(&pool->idle_lock);
	thr = (cthread*)cqueue_pop(pool->idle_queue);
	thr->proc = proc;
	thr->proc_data = proc_data;
	spinlock_unlock(&pool->idle_lock);
	pthread_cond_signal(&thr->cond);
	return 0;
}
