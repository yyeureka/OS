#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <stdint.h> 
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>

// MCS Barrier
typedef struct {
	uint8_t dummy;

    // fan in
    uint8_t *parent;

    union { 
		uint8_t single[4];
		uint32_t all;
	}fan_in;

	union {
		uint8_t single[4];
		uint32_t all;
	}fan_in_not_ready;

    // fan out
    uint8_t *fan_out[2]; 
    uint8_t sense;
    uint8_t global_sense;
} omp_mcs_node;

static omp_mcs_node *nodes;

void gtmp_init(int num_threads){
    nodes = (omp_mcs_node *)malloc(sizeof(omp_mcs_node) * num_threads);
    memset(nodes, 0, sizeof(omp_mcs_node) * num_threads);

    for (int i = 0; i < num_threads; i++) {
        nodes[i].dummy = 0x0;

        // fan in
        nodes[i].parent = (0 != i) ? &nodes[(i - 1) / 4].fan_in_not_ready.single[(i - 1) % 4] : &nodes[i].dummy;
        for (int j = 1; j <= 4; j++) {
            nodes[i].fan_in.single[j - 1] = (4 * i + j < num_threads) ? true : false;
        }
        nodes[i].fan_in_not_ready.all = nodes[i].fan_in.all;

        // fan out
        for (int j = 1; j <= 2; j++) {
            nodes[i].fan_out[j - 1] = (2 * i + j < num_threads) ? &nodes[2 * i + j].sense : &nodes[i].dummy;
        }
        nodes[i].sense = 0x0;
        nodes[i].global_sense = 0x1;
    }
}

void gtmp_barrier(){
    int th_idx;

    th_idx = omp_get_thread_num();

    // fan in
    while (0x0 != nodes[th_idx].fan_in_not_ready.all);
    nodes[th_idx].fan_in_not_ready.all = nodes[th_idx].fan_in.all;
    *nodes[th_idx].parent = 0x0;

    // fan out
    if (0 != th_idx) {
        while (nodes[th_idx].sense != nodes[th_idx].global_sense);
    }
    *nodes[th_idx].fan_out[0] = nodes[th_idx].global_sense;
    *nodes[th_idx].fan_out[1] = nodes[th_idx].global_sense;
    nodes[th_idx].global_sense = !nodes[th_idx].global_sense;
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
  printf("omp_mcs: %f us %d threads %d rounds\n", mean_time, num_threads, num_iter);

  gtmp_finalize();

  return 0;
}