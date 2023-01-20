#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <mpi.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

static int sense;
static unsigned int count;
static unsigned int thread_cnt;
static unsigned int process_cnt;

// Sense reversing Barrier 
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

// Dissemination Barrier
void gtmpi_init(int num_processes){
    process_cnt = num_processes;
}

void gtmpi_barrier(){
    int round;
    int rank;
    MPI_Status status;

    round = ceil(log2(process_cnt));
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int r;
    for (r = 0; r < round; r++) {
        MPI_Send(NULL, 0, MPI_INT, (int)(rank + pow(2, r)) % process_cnt, 0, MPI_COMM_WORLD);
        MPI_Recv(NULL, 0, MPI_INT, (int)(rank - pow(2, r) + process_cnt) % process_cnt, 0, MPI_COMM_WORLD, &status);
    }
}

void gtmpi_finalize(){

}

int main(int argc, char** argv)
{
  int num_threads;
  int num_processes;
  int num_iter = 100;
  double mean_time = 0.0;

  MPI_Init(&argc, &argv);

  if (argc < 3){
    fprintf(stderr, "Usage: ./harness [NUM_PROCS NUM_THREADS]\n");
    exit(EXIT_FAILURE);
  }
  if (argc >= 4) {
      num_iter = strtol(argv[3], NULL, 10);
  }
  num_processes = strtol(argv[1], NULL, 10);
  num_threads = strtol(argv[2], NULL, 10);

  omp_set_dynamic(0);
  if (omp_get_dynamic())
    printf("Warning: dynamic adjustment of threads has been set\n");

  omp_set_num_threads(num_threads);
  
  gtmp_init(num_threads);
  gtmpi_init(num_processes);

  struct timeval t1, t2;
  gettimeofday(&t1, NULL);

#pragma omp parallel shared(num_threads)
  {
    int i;
    for(i = 0; i < num_iter; i++){
      gtmp_barrier();
      if (0 == omp_get_thread_num()) {
        gtmpi_barrier();
      }
      gtmp_barrier();
    }
  }

  gettimeofday(&t2, NULL);
  mean_time = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
  mean_time /= num_iter;
  printf("omp+mpi: %f us %d processes %d threads %d rounds\n", mean_time, num_processes, num_threads, num_iter);

  gtmp_finalize();
  gtmpi_finalize();

  MPI_Finalize();

  return 0;
}