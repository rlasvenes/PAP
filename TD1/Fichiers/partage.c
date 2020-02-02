#include <stdlib.h>
#include <stdio.h>
#include <omp.h>


int
main()
{
  
  int k = 0;

#pragma omp parallel
{
  int i;
  #pragma omp critical
  for(i = 0; i < 100000; i++)
    k++;
 }
 
 printf("nbthreads x 100000 = %d\n ",k);
 return 0;
}
