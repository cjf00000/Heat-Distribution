CC=gcc
CFLAG=-O2 -lX11

seq: main_seq.c display.h models.h const.h
	${CC} main_seq.c -o temperature_seq ${CFLAG}


openmp: main_openmp.c display.h models.h const.h
	${CC} main_openmp.c -o temperature_openmp ${CFLAG} -fopenmp

openmp_increment: main_openmp_increment.c display.h models.h const.h
	${CC} main_openmp_increment.c -o temperature_openmp ${CFLAG} -fopenmp

all: seq openmp

clean:
	rm -f *~
	rm -f temperature_seq
	rm -f temperature_openmp
