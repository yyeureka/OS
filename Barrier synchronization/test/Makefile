OMPFLAGS = -fopenmp
OMPLIBS = -lgomp

CC = gcc
CFLAGS = -g -std=gnu99 -I. -Wall $(OMPFLAGS)
LDLIBS = $(OMPLIBS) -lm

all: omp_cen omp_dis omp_mcs mpi_mcs mpi_dis mpi_tou

omp_cen: omp_centralized.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

omp_dis: omp_dissemination.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

omp_mcs: omp_mcs.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

MPICC = mpicc
CFLAGS_ = -g -Wall -std=gnu99 -I.
LDLIBS_ = -lm

mpi_mcs: mpi_mcs.o
	$(MPICC) -o $@ $(CFLAGS_) $(LDFLAGS) $^ $(LDLIBS_)

mpi_dis: mpi_dissemination.o
	$(MPICC) -o $@ $(CFLAGS_) $(LDFLAGS) $^ $(LDLIBS_)

mpi_tou: mpi_tournament.o
	$(MPICC) -o $@ $(CFLAGS_) $(LDFLAGS) $^ $(LDLIBS_)

clean:
	rm -rf *.o *.dSYM omp_cen omp_dis omp_mcs mpi_mcs mpi_dis mpi_tou