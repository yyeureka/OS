MPICC = mpicc

OMPFLAGS = -fopenmp
OMPLIBS = -lgomp

CFLAGS = -g -std=gnu99 -I. -Wall $(OMPFLAGS)
LDLIBS = $(OMPLIBS) -lm

all: combined 

SRCS := $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

%.o: %.c
	$(MPICC) -c $(CFLAGS) $< -o $@

combined: $(OBJS) 
	$(MPICC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS)

clean:
	rm -rf *.o *.dSYM combined

