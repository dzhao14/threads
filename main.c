
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "int128.h"
#include "factor.h"
#include "ivec.h"

int
main(int argc, char* argv[])
{
    if (argc != 4) {
        printf("Usage:\n");
        printf("  ./main threads start count\n");
        return 1;
    }

    int threads = atoi(argv[1]);
    assert(threads == 1);

    int128_t start = atoh(argv[2]);
    int64_t  count = atol(argv[3]);

    // FIXME: Maybe we're spawning threads in init?
    factor_init(); //creates two queues.
	
	//fill up iqueue
    for (int64_t ii = 0; ii < count; ++ii) {
        factor_job* job = make_job(start + ii);
        submit_job(job);
    }

    // FIXME: This should be (threads) seperate threads.
    run_jobs();

    int64_t oks = 0;

    // FIXME: This should probably be while ((job = get_result()))
    for (int64_t ii = 0; ii < count; ++ii) {
        factor_job* job = get_result();

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
    }

    printf("Factored %ld / %ld numbers.\n", oks, count);

    factor_cleanup();
    // FIXME: We should have joined all spawned threads.

    return 0;
}
