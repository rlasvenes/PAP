#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <omp.h>

#define T 10
int A[T][T+1]; // on a rajouter le +1 pour nous faciliter la "tâche" sans avoir à calculer -1 sur les indices (?)

int k = 0;

void tache(int i, int j)
{
  volatile int x = random() % 1000000;
  for (int z = 0; z < x; z++); 
  
  #pragma omp atomic capture 
  A[i][j] = k++;
}

int main (int argc, char **argv) {
  
  int i, j;

  // génération des taches
  #pragma omp parallel

  #pragma omp single
  for (i = 0; i < T; i++)
    for (j = 0; j < T; j++) {
      
      if (i == 0 /* && (j > 0 && j < T) */) { // 1er cas = 1ere ligne en haut du tableau
        #pragma omp task firstprivate(i,j) depend(out:A[i][j]) depend(in: A[i][j-1])
        tache(i,j);
      }

      else if (i == T /* && (j > 0 && j < T) */) { // 2eme cas = dernière ligne en bas du tableau
        #pragma omp task firstprivate(i,j)  depend(out:A[i][j]) depend(in: A[i][j-1])
        tache(i,j);
      }

      else if (j == 0/*  && (i > 0 && i < T) */) { // 3eme cas = 1ere colonne à gauche du tableau
        #pragma omp task firstprivate(i,j)  depend(out:A[i][j]) depend(in: A[i-1][j])
        tache(i,j);
      }

      else if (j == T /* && (i > 0 && i < T) */) { // 4eme cas = dernière colonne à droitre du tableau
        #pragma omp task firstprivate(i,j)  depend(out:A[i][j]) depend(in: A[i-1][j])
        tache(i,j);
      }
      
      else { // le reste = "milieu" du tableau (qui peut acceder aux cases i-1 et j-1)
        #pragma omp task firstprivate(i,j) depend(in:A[i-1][j], A[i][j-1]) depend(out:A[i][j])
        tache(i,j);
      }
      
    }

  // affichage du tableau 
  for (i = 0; i < T; i++) {
    puts("");
    for (j = 0; j < T; j++)
      printf(" %2d ", A[i][j]) ;
  }

  return 0;
}
