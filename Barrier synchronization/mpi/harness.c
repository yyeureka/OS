#include <stdio.h>
#include <stdlib.h>
#include "gtmpi.h"

int main(int argc, char** argv)
{
  int num_processes, num_rounds = 10;

  MPI_Init(&argc, &argv);
  
  if (argc < 2){
    fprintf(stderr, "Usage: ./harness [NUM_PROCS]\n");
    exit(EXIT_FAILURE);
  }

  num_processes = strtol(argv[1], NULL, 10);

  gtmpi_init(num_processes);
  
  int k;
  for(k = 0; k < num_rounds; k++){
    gtmpi_barrier();
  }

  gtmpi_finalize();  

  MPI_Finalize();

  return 0;
}
