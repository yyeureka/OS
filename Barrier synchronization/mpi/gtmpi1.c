#include <stdlib.h>
#include <stdio.h>
#include "gtmpi.h"

// MCS Barrier
typedef struct {
	int fan_in_children[4];
	int fan_in_parent;
    int fan_out_children[2];
	int fan_out_parent;
} mpi_mcs_node;

static mpi_mcs_node *node;

void gtmpi_init(int num_processes){
    int rank;

    node = (mpi_mcs_node *)malloc(sizeof(mpi_mcs_node));
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (int i = 0; i < 4; i++) {
        node->fan_in_children[i] = (4 * rank + i + 1 < num_processes) ? (4 * rank + i + 1) : -1;
    }

    for (int i = 0; i < 2; i++) {
        node->fan_out_children[i] = (2 * rank + i + 1 < num_processes) ? (2 * rank + i + 1) : -1;
    }

    if (0 == rank) {
        node->fan_in_parent = -1;
        node->fan_out_parent = -1;
    }
    else {
        node->fan_in_parent = (rank - 1) / 4;
        node->fan_out_parent = (rank - 1) / 2;
    }
}

void gtmpi_barrier(){
    int rank;
    MPI_Status status;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (int i = 0; i < 4; i++) {
        if (-1 == node->fan_in_children[i]) {
            continue;
        }

        MPI_Recv(NULL, 0, MPI_INT, node->fan_in_children[i], 0, MPI_COMM_WORLD, &status);
    }

    if (0 != rank) {
        MPI_Send(NULL, 0, MPI_INT, node->fan_in_parent, 0, MPI_COMM_WORLD);
        MPI_Recv(NULL, 0, MPI_INT, node->fan_out_parent, 1, MPI_COMM_WORLD, &status);
    }

    for (int i = 0; i < 2; i++) {
        if (-1 == node->fan_out_children[i]) {
            continue;
        }

        MPI_Send(NULL, 0, MPI_INT, node->fan_out_children[i], 1, MPI_COMM_WORLD);
    }
}

void gtmpi_finalize(){
    free(node);
}
