#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <mpi.h>

#define MAXNUM 100
#define MAXSTR 50
#define MAXRESULTS 10000

char firstHalf[MAXNUM][MAXSTR];
char secondHalf[MAXNUM][MAXSTR];
char strToTest[MAXSTR];
long int result[MAXRESULTS];
int numPrimes = 0;

int isprime(long int value)
{
  long int root;
  long int factor = 2;
  int prime = 1;
  root = sqrtl(value);
  while ((factor <= root) && (prime))
  {
    prime = fmod((double)value, (double)factor) > 0.0;
    factor++;
  }
  return prime;
}

void quicksort(long int *primes, int first, int last)
{
  int i, j, pivot;
  long int temp;

  if (first < last)
  {
    pivot = first;
    i = first;
    j = last;
    while (i < j)
    {
      while (primes[i] <= primes[pivot] && i < last)
        i++;
      while (primes[j] > primes[pivot])
        j--;
      if (i < j)
      {
        temp = primes[i];
        primes[i] = primes[j];
        primes[j] = temp;
      }
    }
    temp = primes[pivot];
    primes[pivot] = primes[j];
    primes[j] = temp;
    quicksort(primes, first, j - 1);
    quicksort(primes, j + 1, last);
  }
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  int numProcesses, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  FILE *primesFile;
  int i = 0, j = 0, numResults = 0;
  long int primeToTest;

  primesFile = stdin;
  if (rank == 0)
  {
    fscanf(primesFile, "%d\n", &numPrimes);
    for (i = 0; i < numPrimes; i++)
      fscanf(primesFile, "%s\n", firstHalf[i]);
    for (i = 0; i < numPrimes; i++)
      fscanf(primesFile, "%s\n", secondHalf[i]);
    fclose(primesFile);
  }

  MPI_Bcast(&numPrimes, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(firstHalf, numPrimes * MAXSTR, MPI_CHAR, 0, MPI_COMM_WORLD);
  MPI_Bcast(secondHalf, numPrimes * MAXSTR, MPI_CHAR, 0, MPI_COMM_WORLD);

  int chunkSize = numPrimes / numProcesses;
  int remainder = numPrimes % numProcesses;
  int start = rank * chunkSize;
  int end = start + chunkSize;

  if (rank == numProcesses - 1)
    end += remainder;

  // Início da medição de tempo
  double start_time = MPI_Wtime();

  for (i = start; i < end; i++)
    for (j = 0; j < numPrimes; j++)
    {
      strcpy(strToTest, firstHalf[i]);
      strcat(strToTest, secondHalf[j]);
      primeToTest = atol(strToTest);
      if (isprime(primeToTest))
      {
        result[numResults++] = primeToTest;
      }
    }

  // Fim da medição de tempo
  double end_time = MPI_Wtime();

  // Tempo total de execução
  double total_time = end_time - start_time;

  // Envia o número de resultados para o processo 0
  MPI_Send(&numResults, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

  if (rank == 0)
  {
    int totalResults = numResults;
    int *recvCounts = (int *)malloc(numProcesses * sizeof(int));
    int *displacements = (int *)malloc(numProcesses * sizeof(int));

    // Recebe o número de resultados de cada processo
    for (i = 0; i < numProcesses; i++)
    {
      MPI_Recv(&recvCounts[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      displacements[i] = i == 0 ? 0 : displacements[i - 1] + recvCounts[i - 1];
      totalResults += recvCounts[i];
    }

    long int *recvResult = (long int *)malloc(totalResults * sizeof(long int));

    // Copia os resultados do processo 0 para o array final
    memcpy(recvResult, result, numResults * sizeof(long int));

    for (i = 1; i < numProcesses; i++)
    {
      long int *tempResult = (long int *)malloc(recvCounts[i] * sizeof(long int));

      // Recebe os resultados dos outros processos
      MPI_Recv(tempResult, recvCounts[i], MPI_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      // Copia os resultados recebidos para o array final
      memcpy(recvResult + displacements[i], tempResult, recvCounts[i] * sizeof(long int));

      free(tempResult);
    }

    quicksort(recvResult, 0, totalResults - 1);
    for (i = 0; i < totalResults; i++)
      printf("%ld\n", recvResult[i]);

    free(recvResult);
    free(recvCounts);
    free(displacements);

    // Imprime o tempo total de execução
    printf("Tempo total de execução: %f segundos\n", total_time);
  }
  else
  {
    // Envia os resultados para o processo 0
    MPI_Send(result, numResults, MPI_LONG, 0, 0, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
