#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include "queue.h"
#include "factor.h"
#include <stdio.h>

// FIXME: Shared mutable data
extern int DONE;
static queue* iqueue;
static queue* oqueue;
pthread_mutex_t iqueue_mutex;
pthread_mutex_t oqueue_mutex;
pthread_cond_t iqueue_cond;
pthread_cond_t oqueue_cond;

void 
factor_init()
{
    if (iqueue == 0) iqueue = make_queue();
    if (oqueue == 0) oqueue = make_queue();
	pthread_mutex_init(&iqueue_mutex, 0);
	pthread_mutex_init(&oqueue_mutex, 0);
	pthread_cond_init(&iqueue_cond, 0);
	pthread_cond_init(&oqueue_cond, 0);
}

void
factor_cleanup(int threads, pthread_t* workers)
{
	for (int i = 0; i < threads; i++) {
		pthread_cond_signal(&iqueue_cond);
	}
	for (int i = 0; i < threads; i++) {
		int rv = pthread_join(workers[i], 0);
		assert(rv == 0);
	}
	pthread_mutex_destroy(&iqueue_mutex);
	pthread_mutex_destroy(&oqueue_mutex);
	pthread_cond_destroy(&oqueue_cond);
	pthread_cond_destroy(&iqueue_cond);
    free_queue(iqueue);
    iqueue = 0;
    free_queue(oqueue);
    oqueue = 0;
}

factor_job* 
make_job(int128_t nn)
{
    factor_job* job = malloc(sizeof(factor_job));
    job->number  = nn;
    job->factors = 0;
    return job;
}

void 
free_job(factor_job* job)
{
    if (job->factors) {
        free_ivec(job->factors);
    }
    free(job);
}

void 
submit_job(factor_job* job)
{
    queue_put(iqueue, job);
}

factor_job* 
get_result()
{
    return queue_get(oqueue);
}

static
int128_t
isqrt(int128_t xx)
{
    double yy = ceil(sqrt((double)xx));
    return (int128_t) yy;
}

ivec*
factor(int128_t xx)
{
    ivec* ys = make_ivec();

    while (xx % 2 == 0) {
        ivec_push(ys, 2);
        xx /= 2;
    }

    for (int128_t ii = 3; ii <= isqrt(xx); ii += 2) {
        int128_t x1 = xx / ii;
        if (x1 * ii == xx) {
            ivec_push(ys, ii);
            xx = x1;
            ii = 1;
        }
    }

    ivec_push(ys, xx);

    return ys;
}

void 
run_jobs()
{
	//printf("thread: %d\n", (int)pthread_self());
    factor_job* job;
	pthread_mutex_lock(&iqueue_mutex);
	while (DONE == 0 && !(job = queue_get(iqueue))) {
		pthread_cond_wait(&iqueue_cond, &iqueue_mutex);
	}
	if (DONE == 1) { 
		pthread_mutex_unlock(&iqueue_mutex);
		return;
	}
	pthread_mutex_unlock(&iqueue_mutex);
	
	assert(job != 0);

    job->factors = factor(job->number);

	pthread_mutex_lock(&oqueue_mutex);
    queue_put(oqueue, job);
	pthread_cond_broadcast(&oqueue_cond);
	pthread_mutex_unlock(&oqueue_mutex);
}
