OMPFLAGS = -fopenmp
OMPLIBS = -lgomp

CC = gcc
CFLAGS = -g -std=gnu99 -I. -Wall $(OMPFLAGS)
LDLIBS = $(OMPLIBS) -lm


MP_SRC1 = gtmp1.c
MP_SRC2 = gtmp2.c

all: mp1 mp2

mp1: gtmp1.c harness.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

mp2: gtmp2.c harness.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf *.o *.dSYM mp1 mp2
