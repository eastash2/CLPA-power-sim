CC=gcc
OBJ = main.o sim.o page.o
HEADER=../include/sim.h ../include/page.h
CFLAGS= -Wall -Iinclude

all: sim
sim: $(OBJ) $(HEADER)
	${CC} $(CFLAGS) $^ -o $@
clean:
	rm -f ./*.o
