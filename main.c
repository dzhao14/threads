#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "int128.h"
#include "factor.h"
#include "ivec.h"

int DONE = 0;
extern pthread_mutex_t oqueue_mutex;
extern pthread_mutex_t iqueue_mutex;
extern pthread_cond_t iqueue_cond;
extern pthread_cond_t oqueue_cond;

void*
factorize_work() {
	while (DONE == 0) {
		run_jobs();
	}
}

int
main(int argc, char* argv[])
{
	printf("main: %d\n", (int)pthread_self());
    if (argc != 4) {
        printf("Usage:\n");
        printf("  ./main threads start count\n");
        return 1;
    }

    int threads = atoi(argv[1]);
	pthread_t workers[threads];
	for (int i = 0; i < threads; i++) {
		int rv = pthread_create(&(workers[i]), 0, factorize_work, 0);
		assert(rv==0);
	}

    int128_t start = atoh(argv[2]);
    int64_t  count = atol(argv[3]);

    // FIXME: Maybe we're spawning threads in init?
    factor_init(); //creates two queues.
	//fill up iqueue
    for (int64_t ii = 0; ii < count; ++ii) {
        factor_job* job = make_job(start + ii);
		pthread_mutex_lock(&iqueue_mutex);
        submit_job(job);
		pthread_cond_broadcast(&iqueue_cond);
		pthread_mutex_unlock(&iqueue_mutex);
    }

    int64_t oks = 0;

	factor_job* job;
    // FIXME: This should probably be while ((job = get_result()))
    for (int64_t ii = 0; ii < count; ++ii) {
		pthread_mutex_lock(&oqueue_mutex);
        while (!(job = get_result())) {
			pthread_cond_wait(&oqueue_cond, &oqueue_mutex);
		}

        print_int128(job->number);
        printf(": ");
        print_ivec(job->factors);
		
		//check if prime facts actually multiply to number
        ivec* ys = job->factors;
        
        int128_t prod = 1;
        for (int ii = 0; ii < ys->len; ++ii) {
            prod *= ys->data[ii];
        }

        if (prod == job->number) {
            ++oks;
        }
        else {
            fprintf(stderr, "Warning: bad factorization");
        }

        free_job(job);
		pthread_mutex_unlock(&oqueue_mutex);
    }

	DONE = 1;
    printf("Factored %ld / %ld numbers.\n", oks, count);

    factor_cleanup(threads, workers);

    return 0;
}
