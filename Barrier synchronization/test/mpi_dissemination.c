#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <sys/time.h>

// Dissemination Barrier
static unsigned int process_cnt;

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
        MPI_Send(NULL, 0, MPI_INT, (int)(rank + pow(2, r)) % process_cnt, 0, MPI_COMM_WORLD);
        MPI_Recv(NULL, 0, MPI_INT, (int)(rank - pow(2, r) + process_cnt) % process_cnt, 0, MPI_COMM_WORLD, &status);
    }
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
  printf("mpi_dis: %f us %d processes %d rounds\n", mean_time, num_processes, num_rounds);

  MPI_Finalize();

  return 0;
}