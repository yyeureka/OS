#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>

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
  int lsense;

  lsense = sense ^ 1;

  if (1 == __sync_fetch_and_sub(&count, 1)) {
    count = thread_cnt;
    sense = lsense;
  }
  else {
    while (sense != lsense);
  }
}

int main(int argc, char** argv)
{
  int num_threads, num_iter = 1000;
  double mean_time = 0.0;

  if (argc < 2){
    fprintf(stderr, "Usage: ./harness [NUM_THREADS]\n");
    exit(EXIT_FAILURE);
  }
  num_threads = strtol(argv[1], NULL, 10);

  omp_set_dynamic(0);
  if (omp_get_dynamic()) {
    printf("Warning: dynamic adjustment of threads has been set\n");
  }

  omp_set_num_threads(num_threads);
  
  gtmp_init(num_threads);

  struct timeval t1, t2;
  gettimeofday(&t1, NULL);

#pragma omp parallel shared(num_threads)
    {
      for(int i = 0; i < num_iter; i++){
        gtmp_barrier();
      }
    }
  
  gettimeofday(&t2, NULL);
  mean_time = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
  mean_time /= num_iter;
  printf("omp_cen: %f us %d threads %d rounds\n", mean_time, num_threads, num_iter);

  return 0;
}

