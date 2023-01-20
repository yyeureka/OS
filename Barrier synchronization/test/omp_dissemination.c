#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#define MAX_ROUND 5

// Dissemination Barrier
typedef struct {
    bool sense;
    bool *next;
} dis_flag;

typedef struct {
    dis_flag flags[2][MAX_ROUND];

    bool global_sense;
    unsigned int parity;
} omp_dis_node;

static omp_dis_node* nodes;
static unsigned int thread_cnt;

void gtmp_init(int num_threads){
    int round;

    thread_cnt = num_threads;
    round = ceil(log2(num_threads));

    nodes = (omp_dis_node *)malloc(sizeof(omp_dis_node) * num_threads);
    memset(nodes, 0, sizeof(omp_dis_node) * num_threads);

    for (int i = 0; i < num_threads; i++) {
        for (int j = 0; j < 2; j++) {
            for (int r = 0; r < round; r++) {
                nodes[i].flags[j][r].next = &(nodes[(int)(i + pow(2, r)) % num_threads].flags[j][r].sense);
            }
        }

        nodes[i].global_sense = true;
    }
}

void gtmp_barrier(){
    int th_idx;
    int round;
    dis_flag *cur;

    th_idx = omp_get_thread_num();
    round = ceil(log2(thread_cnt));

    for (int r = 0; r < round; r++) {
        cur = &nodes[th_idx].flags[nodes[th_idx].parity][r];
        *(cur->next) = nodes[th_idx].global_sense;
        while (cur->sense != nodes[th_idx].global_sense);
    }

    if (1 == nodes[th_idx].parity) {
        nodes[th_idx].global_sense = !nodes[th_idx].global_sense;
    }
    nodes[th_idx].parity ^= 0x1;
}

void gtmp_finalize(){
    free(nodes);
}

int main(int argc, char** argv)
{
  int num_threads, num_iter = 1000; // TODO:
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
  printf("omp_dis: %f us %d threads %d rounds\n", mean_time, num_threads, num_iter);

  gtmp_finalize();

  return 0;
}