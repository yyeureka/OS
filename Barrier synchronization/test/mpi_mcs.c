#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <sys/time.h>

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

int main(int argc, char** argv)
{
  int num_processes, num_rounds = 1000;
  double mean_time = 0.0;

  MPI_Init(&argc, &argv);
  
  if (argc < 2){
    fprintf(stderr, "Usage: ./harness [NUM_PROCS]\n");
    exit(EXIT_FAILURE);
  }
  if (argc >= 3) {
    num_rounds = strtol(argv[2], NULL, 10);
  }
  num_processes = strtol(argv[1], NULL, 10);

  gtmpi_init(num_processes); 

  struct timeval t1, t2;
  gettimeofday(&t1, NULL);
  
  for(int i = 0; i < num_rounds; i++){
    gtmpi_barrier();
  }

  gettimeofday(&t2, NULL);
  mean_time = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
  mean_time /= num_rounds;
  printf("mpi_mcs: %f us %d processes %d rounds\n", mean_time, num_processes, num_rounds);
 
  gtmpi_finalize();  

  MPI_Finalize();

  return 0;
}