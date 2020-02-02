#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <omp.h>


const char *bonjour[]={ "Good morning",
			"Bonjour",
			"Buon Giorno",
			"Buenos días",
			"Egun on",
			NULL};

const char *aurevoir[]={"Bye",
			"Au revoir",
			"Arrivederci",
			"Hasta luego",
			"Adio",
			NULL};
  
int main()
{
   #pragma omp parallel  
  {
   #pragma omp single
    {
      for (int i = 0 ; bonjour[i] != NULL; i++)
      #pragma omp task firstprivate(i) 
      {
          #pragma omp task firstprivate(i) 
          printf("%s (%d)\n",bonjour[i], omp_get_thread_num());    

          #pragma omp task firstprivate(i) depend(in:bonjour[i])
	        printf("%s (%d)\n",aurevoir[i], omp_get_thread_num());
        } 
      }
  } 
  return 0;
}
