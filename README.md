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

    * ### TD 3 et 4

        * notion de tâche avec `pragma omp task`
        * attente de terminaison de tâche avec `pragma omp taskwait`
        * notion de tâches groupées avec `pragma omp taskgroup`
        * notion de dépendances des tâches avec `pragma omp depend`

    * ### TD 5

        * notion de cache (avec le noyau `scrollup`)
        * multiplication de matrices (`mat_mul.c`)
        * version tiled du kernel `rotation90`

    * ### TD 6

        * optimisation de code avec le kernel `blur`
            * distinction entre le calcul des tuiles du bords, et des tuiles internes (`check` et `no-check`).
        * vectorisation du kernel `spin`
            * AVX2, `atan`, `atan2`, ...
        * vectorisation du kernel `mandelbrot`

    * ### TD 7
        * accélerateurs GPU, OpenCL
            * notion de workgroup
            * notion de divergence entre threads


    * # Projet PAP

        * ## 1er rendu (binôme)
            * optimisations des versions `tiled` et `seq`
                * calcul des tuiles de manières paresseuses
                * calcul uniquement des tuiles instables
            * ajout d'une version `OpenMP` + optimisations du code
                * version synchrone et asynchrone (structure damier)
            * production de courbes de speedup avec `./easyplots`

        * ## 2eme rendu (individuel)
            * au choix: `OpenCL`, `MPI` ou travail sur `tsp`

            * ### OpenCL
                * version opencl basique; `ocl_mine`