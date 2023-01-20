#include <mpi.h>
#include <math.h>

#ifndef GTMPI_H
#define GTMPI_H

void gtmpi_init(int num_processes);
void gtmpi_barrier();
void gtmpi_finalize();

#endif
