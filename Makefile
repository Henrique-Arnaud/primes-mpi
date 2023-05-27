FLAGS=-O3 -Wall
OUT=primetest

all: primetest

run: primetest input
	./primetest < input

primetest: primetest.c
	$(CC) primetest.c $(FLAGS) -lm -o $(OUT)

runParallel: main input
	mpirun -np 4 main < input

main: main.c
	mpicc -o main main.c -lm

clean:
	rm $(OUT)
