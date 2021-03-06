CC=gcc
MPICC=mpicc
CFLAG=-O2 -lX11 -lrt -lm

all: openmp_increment mpi_increment pthread_increment

openmp_increment: main_openmp_increment.c display.h models.h const.h
	${CC} main_openmp_increment.c -o temperature_openmp ${CFLAG} -fopenmp

pthread_increment: main_pthread_increment.c display.h models.h const.h
	${CC} main_pthread_increment.c -o temperature_pthread ${CFLAG} -lpthread

mpi_increment: main_mpi_increment.c display.h models.h const.h
	${MPICC} main_mpi_increment.c -o temperature_mpi ${CFLAG} 

clean:
	rm -f *~
	rm -f temperature_openmp
	rm -f temperature_pthread
	rm -f temperature_mpi
