#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "gtmp.h"
// #include <stdint.h> 

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
                // printf("Round %d %d point to %d\n", r, i, (int)(i + pow(2, r)) % num_threads);
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
        // printf("Thread %d round %d arrive\n", th_idx, r);
        while (cur->sense != nodes[th_idx].global_sense);
        // printf("Thread %d round %d pass\n", th_idx, r);
    }

    if (1 == nodes[th_idx].parity) {
        nodes[th_idx].global_sense = !nodes[th_idx].global_sense;
    }
    nodes[th_idx].parity ^= 0x1;
}

void gtmp_finalize(){
    free(nodes);
}


// // MCS Barrier
// typedef struct {
// 	uint8_t dummy;

//     // fan in
//     uint8_t *parent;

//     union { 
// 		uint8_t single[4];
// 		uint32_t all;
// 	}fan_in;

// 	union {
// 		uint8_t single[4];
// 		uint32_t all;
// 	}fan_in_not_ready;

//     // fan out
//     uint8_t *fan_out[2]; 
//     uint8_t sense;
//     uint8_t global_sense;
// } omp_mcs_node;

// static omp_mcs_node *nodes;

// void gtmp_init(int num_threads){
//     nodes = (omp_mcs_node *)malloc(sizeof(omp_mcs_node) * num_threads);
//     memset(nodes, 0, sizeof(omp_mcs_node) * num_threads);

//     for (int i = 0; i < num_threads; i++) {
//         nodes[i].dummy = 0x0;

//         // fan in
//         nodes[i].parent = (0 != i) ? &nodes[(i - 1) / 4].fan_in_not_ready.single[(i - 1) % 4] : &nodes[i].dummy;
//         for (int j = 1; j <= 4; j++) {
//             nodes[i].fan_in.single[j - 1] = (4 * i + j < num_threads) ? true : false;
//         }
//         nodes[i].fan_in_not_ready.all = nodes[i].fan_in.all;

//         // fan out
//         for (int j = 1; j <= 2; j++) {
//             nodes[i].fan_out[j - 1] = (2 * i + j < num_threads) ? &nodes[2 * i + j].sense : &nodes[i].dummy;
//         }
//         nodes[i].sense = 0x0;
//         nodes[i].global_sense = 0x1;
//     }
// }

// void gtmp_barrier(){
//     int th_idx;

//     th_idx = omp_get_thread_num();

//     // fan in
//     while (0x0 != nodes[th_idx].fan_in_not_ready.all);
//     nodes[th_idx].fan_in_not_ready.all = nodes[th_idx].fan_in.all;
//     *nodes[th_idx].parent = 0x0;

//     // fan out
//     if (0 != th_idx) {
//         while (nodes[th_idx].sense != nodes[th_idx].global_sense);
//     }
//     *nodes[th_idx].fan_out[0] = nodes[th_idx].global_sense;
//     *nodes[th_idx].fan_out[1] = nodes[th_idx].global_sense;
//     nodes[th_idx].global_sense = !nodes[th_idx].global_sense;
// }

// void gtmp_finalize(){
//     free(nodes);
// }

