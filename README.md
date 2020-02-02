# PAP

## UE Programmation des architectures parallèles
Voici les différentes notions étudiées :
* OpenMP 4.X+
    * ### TD 1 et 2
        * directives `pragma omp parallel` pour créer une équipe de threads.
        * section critique avec `pragma omp critical`
        * barrière implicite/explicite avec `pragma omp barrier`
        * bloc d'instructions atomique avec `pragma omp atomic`
        * directive `pragma omp reduction` (utile quand on veut sommer des éléments d'un tableau par exemple)
        * les politiques d'attributions d'indices d'une boucle for avec `pragma omp for schedule(runtime | static | dynamic | guided)`
        * attributions de couples d'indices dans une boucle for avec `pragma omp for collapse(N)` où `N` = nb de boucle for imbriqué
        * les variables partagées avec `firstprivate`, `private` et `shared`
        * notions de maître/esclave avec `pragma omp master` et `pragma omp single`

    * ### TD 3

        * notion de tâche avec `pragma omp task`
        * attente de terminaison de tâche avec `pragma omp taskwait`
        * notion de tâches groupées avec `pragma omp taskgroup`
        * notion de dépendances des tâches avec `pragma omp depend`