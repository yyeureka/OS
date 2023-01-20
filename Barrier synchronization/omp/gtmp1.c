#include "gtmp.h"

// Sense reversing Barrier 
static int sense;
static unsigned int count;
static unsigned int thread_cnt;

void gtmp_init(int num_threads){
    thread_cnt = num_threads;
    count = num_threads;
    sense = 0;
}

void gtmp_barrier(){
    // int th_idx = omp_get_thread_num();
    int lsense;

    lsense = sense ^ 1;

    if (1 == __sync_fetch_and_sub(&count, 1)) {
        // printf("Thread %d close\n", th_idx);
        count = thread_cnt;
        sense = lsense;
    }
    else {
        // printf("Thread %d wait\n", th_idx);
        while (sense != lsense);
    }
}

void gtmp_finalize(){

}

