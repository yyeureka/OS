#include <stdlib.h>
#include <stdio.h>
#include "gtmpi.h"

// Tournament Barrier
static unsigned int process_cnt;

int bit_pow(int base, int num){
    int result = 1;

    while (num) {
        if (num & 1) {
            result *= base;
        }

        num >>= 1;
        base *= base;
    }

    return result;
}

void gtmpi_init(int num_processes){
    process_cnt = num_processes;
}

void gtmpi_barrier(){
    int round;
    int rank;
    MPI_Status status;

    round = ceil(log2(process_cnt));
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (int r = 0; r < round; r++) {
        if (0 == (rank % bit_pow(2, r + 1))) {
            if (rank + bit_pow(2, r) < process_cnt) {
                MPI_Recv(NULL, 0, MPI_INT, rank + bit_pow(2, r), 0, MPI_COMM_WORLD, &status);
            }
        }
        else {
            MPI_Send(NULL, 0, MPI_INT, rank - bit_pow(2, r), 0, MPI_COMM_WORLD);
            MPI_Recv(NULL, 0, MPI_INT, rank - bit_pow(2, r), 0, MPI_COMM_WORLD, &status);
            break;
        }
    }

    for (int r = round - 1; r >= 0; r--) {
        if (0 != (rank % bit_pow(2, r + 1))) {
            continue;
        }

        if (rank + bit_pow(2, r) < process_cnt) {
            MPI_Send(NULL, 0, MPI_INT, rank + bit_pow(2, r), 0, MPI_COMM_WORLD);
        }
    }
}

void gtmpi_finalize(){

}


// // Dissemination Barrier
// static unsigned int process_cnt;

// void gtmpi_init(int num_processes){
//     process_cnt = num_processes;
// }

// void gtmpi_barrier(){
//     int round;
//     int rank;
//     MPI_Status status;

//     round = ceil(log2(process_cnt));
//     MPI_Comm_rank(MPI_COMM_WORLD, &rank);

//     for (int r = 0; r < round; r++) {
//         MPI_Send(NULL, 0, MPI_INT, (int)(rank + pow(2, r)) % process_cnt, 0, MPI_COMM_WORLD);
//         MPI_Recv(NULL, 0, MPI_INT, (int)(rank - pow(2, r) + process_cnt) % process_cnt, 0, MPI_COMM_WORLD, &status);
//     }
// }

// void gtmpi_finalize(){

// }
