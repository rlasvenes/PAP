#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <omp.h>

void generer()
{
  char chaine[] = "jusqu'ici tout va bien";
  
  for(int i = 0; i < 20 ; i++)
    #pragma omp task firstprivate(i,chaine)
    printf("tache %2.d par thread %2.d >>>> %s <<<< \n",i,omp_get_thread_num(), chaine);
    // si on enlève le taskwait, on peut sortir de la fonction sans avoir générer toutes les tâches
    // ducoup le StackPointer (SP) "reviens" au main et le contexte de generer() est plus accessible dans 
    // la pile. (on pointe sur une variable qui n'existe plus)
    #pragma omp taskwait
}


int main()
{  
  #pragma omp parallel
  {
    #pragma omp single
    {
      generer();
      printf("%d est sorti de generer() \n", omp_get_thread_num());
    }
  }

  return 0;
}
  
