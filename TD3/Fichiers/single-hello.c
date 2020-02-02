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
  
// chaque tâches créés est mise dans un "sac" commun
// taskwait = j'attend que les tâches que j'ai (moi, le thread) créés soient terminés
int main()
{
	#pragma omp parallel  
	{
		// un seul thread va s'occuper de la création des tâches, d'où la directive "single".
		// ça sert à rien de mettre toutes l'équipes sur la création des tâches sinon on se retrouverais avec
		// trop de tâches (48x trop si 48 threads)
		#pragma omp single
		{
			int single = omp_get_thread_num();

			for (int i = 0 ; bonjour[i] != NULL; i++) {  
				// on met la variable i en firsprivate car d'une éxécution de tâche à un autre,
				// la valeur de i peut avoir changer, or avec firsprivate on s'assure de faire une
				// copie de la variable i. (sinon, pas sûr qu'on est tous les bonjour et au-revoir)
				#pragma omp task firstprivate(i) 
				printf("%s (get = %d -- single = %d)\n",bonjour[i], omp_get_thread_num(), single);
				
			}

			for (int i = 0 ; aurevoir[i] != NULL; i++) {
				#pragma omp task firstprivate(i)
				printf("%s (get = %d -- single = %d)\n",aurevoir[i], omp_get_thread_num(), single);
				
			}
		}
	}	
	return 0;
}
