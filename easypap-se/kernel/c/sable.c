#include "easypap.h"

#include <stdbool.h>

#include <omp.h>


/**
 * README (rendu précédent en binôme):
 * Fichier sable.c final comprenant au total 3 versions optimisées:
 * 
 * 1) tiled_asynch_damier
 *    cmdline :   ./run -k sable -v tiled_asynch_damier -a 4partout -s 4096 -g 32 -i 1000 -n
 * 
 * 2) tiled_synch_opt
 *    cmdline :   ./run -k sable -v tiled_synch_opt     -a 4partout -s 4096 -g 32 -i 1000 -n
 * 
 * 3) tiled_synch_damier
 *    cmdline :   ./run -k sable -v tiled_synch_damier  -a 4partout -s 4096 -g 32 -i 1000 -n
 * 
 * 
 * 
 * La version 1) correspond à la première version, où on s'éboule sur ses voisins,
 * avec la stratégie damier, où on traite 4 tuiles par itérations, combinés à OpenMP.
 * 
 * La version 2) correspond à la version synchrone, où nos voisins s'éboulent sur nous
 * et où on retranche nos grains qui s'éboulent. Elle utilise aussi une tableau
 * de booléen afin de ne pas recalculer des tuiles stables, combinés à OpenMP.
 * 
 * La version 3) est très similaire à la version 1), si ce n'est que c'est la version 
 * synchrone.
 */


/**
 * REAME (rendu individuel):
 * 
 * Le fichier contiens une version "ocl_mine" et "ocl_opti" (qui marche pas vraiment faute de temps).
 * Il contient également les versions précédentes, citées ci-dessus, au détail prêt qu'il faut remplacer 
 * le code de la fonction "sable_refresh_img" par le bloc juste au dessus de cette dernière.
 */

// Utiliser pour les variantes CPU
static long unsigned int *TABLE = NULL;
static long unsigned int *TABLE_TMP = NULL;
static bool *TABLE_CHANGED = NULL;
static bool *TABLE_CHANGED_TMP = NULL;

// Utiliser pour la version OpenCL (GPU)
static uint32_t *TABLE_OCL = NULL;

static volatile int changement;
static unsigned long int max_grains;

#define table(i, j)             TABLE             [(i)  *   DIM   + (j)]
#define table_ocl(i, j)         TABLE_OCL         [(i)  *   DIM   + (j)]
#define table_tmp(i, j)         TABLE_TMP         [(i)  *   DIM   + (j)]
#define table_changed(i, j)     TABLE_CHANGED     [(i)  * (GRAIN) + (j)]
#define table_changed_tmp(i, j) TABLE_CHANGED_TMP [(i)  * (GRAIN) + (j)]

#define RGB(r, v, b) (((r) << 24 | (v) << 16 | (b) << 8) | 255)

static inline void init_tables(void);

void sable_init ()
{
  TABLE = calloc (DIM * DIM, sizeof (long unsigned int));
  TABLE_OCL = calloc (DIM * DIM, sizeof (uint32_t));
  TABLE_TMP = calloc (DIM * DIM, sizeof (long unsigned int));
  TABLE_CHANGED = calloc(GRAIN*GRAIN, sizeof (bool));
  TABLE_CHANGED_TMP = calloc(GRAIN*GRAIN, sizeof (bool));
  init_tables();

}

void sable_finalize ()
{
  free(TABLE);
  free(TABLE_OCL);
  free(TABLE_TMP);
  free(TABLE_CHANGED);
}

/*
Copier-coller le contenu de la fonction ci-dessus dans la fonction du même nom (mais
non-commenté).

void sable_refresh_img ()
{
  unsigned long int max = 0;
  for (int i = 1; i < DIM - 1; i++)
    for (int j = 1; j < DIM - 1; j++) {
      int g = table (i, j);
      int r, v, b;
      r = v = b = 0;
      if (g == 1)
        v = 255;
      else if (g == 2)
        b = 255;
      else if (g == 3)
        r = 255;
      else if (g == 4)
        r = v = b = 255;
      else if (g > 4)
        r = b = 255 - (240 * ((double)g) / (double)max_grains);

      cur_img (i, j) = RGB (r, v, b);
      if (g > max)
        max = g;
    }
  max_grains = max;
}
*/

///////////////////////////// Production d'une image
// La fonction se nomme ainsi car cela posé problème autrement, 
// si elle s'appelle "sable_refresh_img" alors qu'on a 
// définit une autre fonction "sable_refresh_img_ocl", cette 
// dernière n'est jamais appelé.
void refresh_img ()
{
  unsigned long int max = 0;
  for (int i = 1; i < DIM - 1; i++)
    for (int j = 1; j < DIM - 1; j++) {
      int g = table_ocl (i, j); // Attention, on lis bien dans TABLE_OCL et pas TABLE
      int r, v, b;
      r = v = b = 0;
      if (g == 1)
        v = 255;
      else if (g == 2)
        b = 255;
      else if (g == 3)
        r = 255;
      else if (g == 4)
        r = v = b = 255;
      else if (g > 4)
        r = b = 255 - (240 * ((double)g) / (double)max_grains);

      cur_img (i, j) = RGB (r, v, b);
      if (g > max)
        max = g;
    }
  max_grains = max;
}

void sable_refresh_img ()
{
  cl_int err;

  err = clEnqueueReadBuffer (queue, cur_buffer, CL_TRUE, 0, sizeof (unsigned) * DIM * DIM, TABLE_OCL, 0, NULL, NULL);       
  check (err, "Failed to read buffer from GPU");

  refresh_img ();
}

///////////////////////////// Configurations initiales

static void sable_draw_4partout (void);

void sable_draw (char *param)
{
  // Call function ${kernel}_draw_${param}, or default function (second
  // parameter) if symbol not found
  hooks_draw_helper (param, sable_draw_4partout);
}

void sable_draw_4partout (void)
{
  max_grains = 8;
  for (int i = 1; i < DIM - 1; i++)
    for (int j = 1; j < DIM - 1; j++) {
      cur_img (i, j) = next_img(i, j) = 4;
      table (i, j) = table_ocl (i, j) = table_tmp(i, j) = 4;
    }
      
}

void sable_draw_DIM (void)
{
  max_grains = DIM;
  for (int i = DIM / 4; i < DIM - 1; i += DIM / 4)
    for (int j = DIM / 4; j < DIM - 1; j += DIM / 4) {
      cur_img (i, j) = next_img(i, j) = i * j / 4;
      table (i, j) = table_ocl (i, j) = table_tmp(i, j) = i * j / 4;
    }
}

void sable_draw_alea (void)
{
  max_grains = 5000;
  for (int i = 0; i<DIM>> 3; i++) {
    int i = 1 + random () % (DIM - 2);
    int j = 1 + random () % (DIM - 2);
    int grains = 1000 + (random () % (4000));
    cur_img (i, j) = next_img(i, j) = grains;
    table (i, j) = table_ocl (i, j) = table_tmp(i, j) = grains;
  }
}

void sable_draw_maxcenter(void)
{
  max_grains = 1;
  for (int i = 1; i < DIM - 1; i++)
    for (int j = 1; j < DIM - 1; j++)
      cur_img (i, j) = table (i, j) = 0;

  cur_img (DIM/2, DIM/2) = table (DIM/2, DIM/2) = 100;
}

///////////////////////////// Version séquentielle simple (seq)
// Version Asynchrone
static inline void compute_new_state (int y, int x)
{
  if (table (y, x) >= 4) {
    unsigned long int div4 = table (y, x) / 4;
    table (y, x - 1) += div4;
    table (y, x + 1) += div4;
    table (y - 1, x) += div4;
    table (y + 1, x) += div4;
    table (y, x) %= 4;
    changement = 1;
  }
}

static void do_tile (int x, int y, int width, int height, int who)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", x, x + width - 1, y,
               y + height - 1);

  monitoring_start_tile (who);

  for (int i = y; i < y + height; i++)
    for (int j = x; j < x + width; j++) {
      compute_new_state (i, j);
    }
  monitoring_end_tile (x, y, width, height, who);
}

// Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
unsigned sable_compute_seq (unsigned nb_iter)
{

  for (unsigned it = 1; it <= nb_iter; it++) {
    changement = 0;
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    do_tile (1, 1, DIM - 2, DIM - 2, 0);
    if (changement == 0)
      return it;
  }
  return 0;
}

///////////////////////////// Version séquentielle tuilée (tiled)

unsigned sable_compute_tiled (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {
    changement = 0;

    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = 0; x < DIM; x += TILE_SIZE)
        do_tile (x + (x == 0), y + (y == 0),
                 TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                 TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                 0 /* CPU id */);
    if (changement == 0)
      return it;
  }

  return 0;
}
/////////////////////////////////////////////////////////////////////////
//-- MODIF
unsigned int inline coord_to_tile (unsigned int coord){
  return coord / TILE_SIZE;
}

static inline void init_tables(void){
  for(int i=0;i<GRAIN;i++){
    for(int j=0;j<GRAIN;j++){
      table_changed(i,j) = true;
      table_changed_tmp(i,j) = true;
    }
  }
  for(int i=0;i<DIM;i++){
    for(int j=0;j<DIM;j++){
      table_tmp(i,j) = table(i,j) = 4;
    }
  }
}

static inline bool compute_new_state_alt (int y, int x)
{
  int surplus, soustrait;
  surplus = 0;
  soustrait = table_tmp(y, x) - (table_tmp(y, x) % 4);

  if(y > 0){
    if(y < DIM-1){
      surplus += table_tmp(y + 1, x) / 4;
    }
    surplus += table_tmp(y - 1, x) / 4;
  }
  if(x > 0){
    if(x < DIM-1){
      surplus += table_tmp(y, x + 1) / 4;
    }
    surplus += table_tmp(y, x - 1) / 4;
  }

  table(y, x) = table_tmp(y, x) + surplus - soustrait;

  if(table(y, x) == table_tmp(y, x)){
    return false;
  }
  return true;
}

static inline bool compute_new_state_alt_inner (int y, int x)
{
  int surplus, soustrait;
  surplus = 0;
  soustrait = table_tmp(y, x) - (table_tmp(y, x) % 4);
  
  surplus += table_tmp(y - 1, x) / 4;
  surplus += table_tmp(y + 1, x) / 4;
  surplus += table_tmp(y, x - 1) / 4;
  surplus += table_tmp(y, x + 1) / 4;

  table(y, x) = table_tmp(y, x) + surplus - soustrait;

  if(table(y, x) == table_tmp(y, x)){
    return false;
  }
  return true;
}

static bool do_tile_opt (int x, int y, int width, int height, int who)
{
  PRINT_DEBUG ('c', "tuile inner [%d-%d][%d-%d] traitée\n", x, x + width - 1, y, y + height - 1);

  monitoring_start_tile (who);

  int i_max = y + height - ((y + height) == DIM) + (y == 0);
  int j_max = x + width - ((x + width) == DIM) + (x == 0);

  bool change = false;

  for (int i = y + (y == 0); i < i_max; i++) {
    for (int j = x + (x == 0); j < j_max; j++) {
      change |= compute_new_state_alt (i, j);
    }
  }

  table_changed(coord_to_tile(y), coord_to_tile(x)) = change;

  monitoring_end_tile (x, y, width, height, who);
  
  return change;
}

static bool do_tile_opt_inner (int x, int y, int width, int height, int who)
{
  PRINT_DEBUG ('c', "tuile inner [%d-%d][%d-%d] traitée\n", x, x + width - 1, y, y + height - 1);

  monitoring_start_tile (who);

  bool change = false;

  for (int i = y; i < y + height; i++) {
    for (int j = x; j < x + width; j++) {
      change |= compute_new_state_alt_inner (i, j);
    }
  }

  table_changed(coord_to_tile(y), coord_to_tile(x)) = change;

  monitoring_end_tile (x, y, width, height, who);

  return change;
}

//------------------------
// Gestion changement futur

bool should_i_recompute(int y, int x){
  bool test_v_left = (x > 0) && table_changed_tmp(y, x - 1);
  bool test_v_right = (x < GRAIN-2) && table_changed_tmp(y, x + 1);
  bool test_v_up = (y > 0) && table_changed_tmp(y - 1, x);
  bool test_v_down = (y < GRAIN-2) && table_changed_tmp(y + 1, x);
  bool test_v_me = table_changed_tmp(y, x);

  return test_v_me || test_v_left || test_v_right || test_v_up || test_v_down;
}

bool should_i_recompute_inner(int y, int x){
  bool test_v_left = table_changed_tmp(y, x - 1);
  bool test_v_right = table_changed_tmp(y, x + 1);
  bool test_v_up = table_changed_tmp(y - 1, x);
  bool test_v_down = table_changed_tmp(y + 1, x);
  bool test_v_me = table_changed_tmp(y, x);

  return test_v_me || test_v_left || test_v_right || test_v_up || test_v_down;
}

//------------------------

static inline void swap_tables() {
  long unsigned int *tmp = TABLE;
  TABLE = TABLE_TMP;
  TABLE_TMP = tmp;

  bool* tmp2 = TABLE_CHANGED;
  TABLE_CHANGED = TABLE_CHANGED_TMP;
  TABLE_CHANGED_TMP = tmp2;
}

unsigned sable_compute_tiled_opt (unsigned nb_iter)
{
  bool changed_tile;
  for (unsigned it = 1; it <= nb_iter; it++) {
    changed_tile = false;
    
    swap_tables();

    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = 0; x < DIM; x += TILE_SIZE)
        if(x == 0 || y == 0 || (x + TILE_SIZE) == DIM || (y + TILE_SIZE) == DIM){
          if(should_i_recompute(coord_to_tile(y), coord_to_tile(x))){
            changed_tile |= do_tile_opt(x, y, TILE_SIZE, TILE_SIZE, 0);
          }
        }else{
          if(should_i_recompute_inner(coord_to_tile(y), coord_to_tile(x))){
            changed_tile |= do_tile_opt_inner(x, y, TILE_SIZE, TILE_SIZE, 0);
          }
        }
    
    if (!changed_tile)
      return it;
  }
  return 0;
}

unsigned sable_compute_tiled_synch_opt (unsigned nb_iter)
{
  bool changed_tile;
  for (unsigned it = 1; it <= nb_iter; it++) {
    changed_tile = false;

    swap_tables();

    #pragma omp parallel for collapse(2) reduction(|:changed_tile) schedule(runtime)
    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = 0; x < DIM; x += TILE_SIZE)
        if(x == 0 || y == 0 || (x + TILE_SIZE) == DIM || (y + TILE_SIZE) == DIM){
          if(should_i_recompute(coord_to_tile(y), coord_to_tile(x))){
            changed_tile |= do_tile_opt(x, y, TILE_SIZE, TILE_SIZE, omp_get_thread_num());
          }
        }else{
          if(should_i_recompute_inner(coord_to_tile(y), coord_to_tile(x))){
            changed_tile |= do_tile_opt_inner(x, y, TILE_SIZE, TILE_SIZE, omp_get_thread_num());
          }
        }

    if (!changed_tile)
      return it;
  }
  return 0;
}

unsigned sable_compute_tiled_task (unsigned nb_iter)
{
  bool changed_tile;
  for (unsigned it = 1; it <= nb_iter; it++) {
    changed_tile = false;
    
    memcpy(TABLE_TMP, TABLE, sizeof(long unsigned int) * DIM * DIM);

    #pragma omp parallel
    #pragma omp single
    {
    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = 0; x < DIM; x += TILE_SIZE)
        if(x == 0 || y == 0 || (x + TILE_SIZE) == DIM || (y + TILE_SIZE) == DIM){
          #pragma omp task firstprivate(x,y)
          {
            changed_tile |= do_tile_opt(x, y, TILE_SIZE, TILE_SIZE, omp_get_thread_num());
          }
        }else{
          #pragma omp task firstprivate(x,y)
          {
            changed_tile |= do_tile_opt_inner(x, y, TILE_SIZE, TILE_SIZE, omp_get_thread_num());
          }
        }
    }
    #pragma omp taskwait

    if (!changed_tile)
      return it;
  }
  return 0;
}

//------------------------
static inline int compute_new_state_ret (int y, int x)
{
  table(y, x) = table_tmp(y, x) % 4;
  table(y, x) += table_tmp(y - 1, x) / 4;
  table(y, x) += table_tmp(y + 1, x) / 4;
  table(y, x) += table_tmp(y, x - 1) / 4;
  table(y, x) += table_tmp(y, x + 1) / 4;

  if(table(y, x) != table_tmp(y, x)){
    return 1;
  }

  return 0;
}

static int do_tile_synch (int x, int y, int width, int height, int who)
{
  PRINT_DEBUG ('c', "tuile inner [%d-%d][%d-%d] traitée\n", x, x + width - 1, y, y + height - 1);
  int changed = 0;
  monitoring_start_tile (who);

  for (int i = y; i < y + height; i++) {
    for (int j = x; j < x + width; j++) {
      changed |= compute_new_state_ret (i, j);
    }
  }

  monitoring_end_tile (x, y, width, height, who);
  
  return changed; 
}

unsigned sable_compute_tiled_synch_damier (unsigned nb_iter)
{
  int changed = false;

  for (unsigned it = 1; it <= nb_iter; it++) {
    swap_tables();
    
    #pragma omp parallel reduction (|:changed)
    {
      //printf("\n Iteration: %d, Thread: %d\n", nb_iter, omp_get_thread_num());
      changed = 0;
      
      #pragma omp for collapse(2) schedule(runtime)
      for (int y = 0; y < DIM; y += (TILE_SIZE*2)) {
        for (int x = 0; x < DIM; x += (TILE_SIZE*2)) {
          if(should_i_recompute(coord_to_tile(y + (y == 0)), coord_to_tile(x + (x == 0)))){
            changed = changed | do_tile_synch (x + (x == 0), y + (y == 0),
                                  TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                                  TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                                  omp_get_thread_num());
          }
        }
      }

      #pragma omp for collapse(2) schedule(runtime)
      for (int y = TILE_SIZE; y < DIM; y += (TILE_SIZE*2)) {
        for (int x = 0; x < DIM; x += (TILE_SIZE*2)) {
          if(should_i_recompute(coord_to_tile(y + (y == 0)), coord_to_tile(x + (x == 0)))){
            changed = changed | do_tile_synch (x + (x == 0), y + (y == 0),
                                  TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                                  TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                                  omp_get_thread_num());
          }
        }
      }

      #pragma omp for collapse(2) schedule(runtime)
      for (int y = TILE_SIZE; y < DIM; y += (TILE_SIZE*2)) {
        for (int x = TILE_SIZE; x < DIM; x += (TILE_SIZE*2)) {
          if(should_i_recompute(coord_to_tile(y + (y == 0)), coord_to_tile(x + (x == 0)))){
            changed = changed | do_tile_synch (x + (x == 0), y + (y == 0),
                                  TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                                  TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                                  omp_get_thread_num());
          }
        }
      }

      #pragma omp for collapse(2) schedule(runtime)
      for (int y = 0; y < DIM; y += (TILE_SIZE*2)) {
        for (int x = TILE_SIZE; x < DIM; x += (TILE_SIZE*2)) {
          if(should_i_recompute(coord_to_tile(y + (y == 0)), coord_to_tile(x + (x == 0)))){
            changed = changed | do_tile_synch (x + (x == 0), y + (y == 0),
                                  TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                                  TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                                  omp_get_thread_num());
          }
        }
      }
    } // fin pragma omp parallel
    if (changed == 0)
      return it;
  }
  return 0;
}

//--
static inline bool compute_new_state_asynch (int y, int x)
{
  if (table (y, x) >= 4) {
    unsigned long int div4 = table (y, x) / 4;
    table (y, x - 1) += div4;
    table (y, x + 1) += div4;
    table (y - 1, x) += div4;
    table (y + 1, x) += div4;
    table (y, x) %= 4;
    return true;
  }
  return false;
}

static int do_tile_asynch (int x, int y, int width, int height, int who)
{
  PRINT_DEBUG ('c', "tuile inner [%d-%d][%d-%d] traitée\n", x, x + width - 1, y, y + height - 1);
  int changed = 0;
  monitoring_start_tile (who);

  for (int i = y; i < y + height; i++) {
    for (int j = x; j < x + width; j++) {
      changed |= compute_new_state_asynch (i, j);
    }
  }

  monitoring_end_tile (x, y, width, height, who);
  
  return changed; 
}


unsigned sable_compute_tiled_asynch_damier (unsigned nb_iter) {
  int changed = false;

  for (unsigned it = 1; it <= nb_iter; it++) {
    swap_tables();
    
    #pragma omp parallel reduction (|:changed)
    {
      //printf("\n Iteration: %d, Thread: %d\n", nb_iter, omp_get_thread_num());
      changed = 0;
      
      #pragma omp for collapse(2) schedule(runtime)
      for (int y = 0; y < DIM; y += (TILE_SIZE*2)) {
        for (int x = 0; x < DIM; x += (TILE_SIZE*2)) {
          if(should_i_recompute(coord_to_tile(y + (y == 0)), coord_to_tile(x + (x == 0)))){
            changed = changed | do_tile_asynch (x + (x == 0), y + (y == 0),
                                  TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                                  TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                                  omp_get_thread_num());
          }
        }
      }

      #pragma omp for collapse(2) schedule(runtime)
      for (int y = TILE_SIZE; y < DIM; y += (TILE_SIZE*2)) {
        for (int x = 0; x < DIM; x += (TILE_SIZE*2)) {
          if(should_i_recompute(coord_to_tile(y + (y == 0)), coord_to_tile(x + (x == 0)))){
            changed = changed | do_tile_asynch (x + (x == 0), y + (y == 0),
                                  TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                                  TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                                  omp_get_thread_num());
          }
        }
      }

      #pragma omp for collapse(2) schedule(runtime)
      for (int y = TILE_SIZE; y < DIM; y += (TILE_SIZE*2)) {
        for (int x = TILE_SIZE; x < DIM; x += (TILE_SIZE*2)) {
          if(should_i_recompute(coord_to_tile(y + (y == 0)), coord_to_tile(x + (x == 0)))){
            changed = changed | do_tile_asynch (x + (x == 0), y + (y == 0),
                                  TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                                  TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                                  omp_get_thread_num());
          }
        }
      }

      #pragma omp for collapse(2) schedule(runtime)
      for (int y = 0; y < DIM; y += (TILE_SIZE*2)) {
        for (int x = TILE_SIZE; x < DIM; x += (TILE_SIZE*2)) {
          if(should_i_recompute(coord_to_tile(y + (y == 0)), coord_to_tile(x + (x == 0)))){
            changed = changed | do_tile_asynch (x + (x == 0), y + (y == 0),
                                  TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                                  TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                                  omp_get_thread_num());
          }
        }
      }
    } // fin pragma omp parallel
    if (changed == 0)
      return it;
  }
  return 0;
}


////////////////////////////////////////////////////////////////
//
//                VERSION OPENCL TAS DE SABLE
//
////////////////////////////////////////////////////////////////

static cl_mem mask_buffer = 0;
static cl_mem changed_buffer = 0;
static cl_mem changed_buffer2 = 0;

void malloc_tables ()
{
  TABLE = calloc (DIM * DIM, sizeof (long unsigned int));
  TABLE_OCL = calloc (DIM * DIM, sizeof (uint32_t));
  TABLE_TMP = calloc (DIM * DIM, sizeof (long unsigned int));
  TABLE_CHANGED = calloc(GRAIN*GRAIN, sizeof (bool));
  TABLE_CHANGED_TMP = calloc(GRAIN*GRAIN, sizeof (bool));
  init_tables();

}

void free_tables ()
{
  free(TABLE);
  free(TABLE_TMP);
  free(TABLE_CHANGED);
}

void sable_init_ocl_mine (void)
{
  malloc_tables();

  changed_buffer = clCreateBuffer (context, CL_MEM_READ_WRITE, sizeof(int), NULL, NULL);
  if (!changed_buffer)
    exit_with_error ("Failed to allocate changed buffer");

}

void sable_init_ocl_opti (void)
{
  malloc_tables();

  // Number of workgroup: (workgroups <=> tiles)
  const int size = (DIM/TILEX) * (DIM/TILEY) * sizeof (unsigned);
  mask_buffer = clCreateBuffer (context, CL_MEM_READ_WRITE, size, NULL, NULL);
  if (!mask_buffer)
    exit_with_error ("Failed to allocate mask buffer");

  changed_buffer2 = clCreateBuffer (context, CL_MEM_READ_WRITE, sizeof(int), NULL, NULL);
  if (!changed_buffer2)
    exit_with_error ("Failed to allocate changed buffer");

}

void sable_finalize_ocl_mine (void)
{
  free_tables();
}

void sable_finalize_ocl_opti (void)
{
  free_tables();
}

void print_table() {
  for (int i = 0; i < DIM-1; i++) {
    for (int j = 0; j < DIM-1; j++) {
      printf("%d ", table_ocl(i, j));
    }
    printf("\n");
  }
}

unsigned sable_invoke_ocl_mine (unsigned nb_iter)
{
  size_t global[2] = {SIZE, SIZE}; // global domain size for our calculation
  size_t local[2]  = {TILEX, TILEY}; // local domain size for our calculation
  cl_int err;

  int changed;

  for (unsigned it = 1; it <= nb_iter; it++) {
    // Swap buffers
    changed = 0;
    
    // Writing to GPU memory (so the kernel will have access to it)
    // Needed to know when to stop the program.
    err = clEnqueueWriteBuffer(queue, changed_buffer, CL_TRUE, 0, sizeof(int), &changed, 0, NULL, NULL);
    check (err, "Failed to write changed");

    // Set kernel arguments
    err = 0;
    err |= clSetKernelArg (compute_kernel, 0, sizeof (cl_mem), &next_buffer);
    err |= clSetKernelArg (compute_kernel, 1, sizeof (cl_mem), &cur_buffer);
    err |= clSetKernelArg (compute_kernel, 2, sizeof (cl_mem), &changed_buffer);
    check (err, "Failed to set kernel arguments");

    err = clEnqueueNDRangeKernel (queue, compute_kernel, 2, NULL, global, local, 0, NULL, NULL);
    check (err, "Failed to execute kernel");

    // Reading from GPU memory (does the tile changed ?)
    err = clEnqueueReadBuffer (queue, changed_buffer, CL_TRUE, 0, sizeof (int), &changed, 0, NULL, NULL);
    check (err, "Failed to read changed");
  
    // If we're here, it means that the sandpile is stable, and that the result
    // is located at the GPU memory, so we store it in our table (CPU memory).
    if (changed == 0) {
      return it;
    }

    {
      cl_mem tmp  = cur_buffer;
      cur_buffer  = next_buffer;
      next_buffer = tmp;
    }

  }

  return 0;
}

unsigned sable_invoke_ocl_opti (unsigned nb_iter)
{
  size_t global[2] = {SIZE, SIZE}; // global domain size for our calculation
  size_t local[2]  = {TILEX, TILEY}; // local domain size for our calculation
  cl_int err;

  int changed;

  for (unsigned it = 1; it <= nb_iter; it++) {
    // Swap buffers
    changed = 0;
    
    // Writing to GPU memory (so the kernel will have access to it)
    // Needed to know when to stop the program.
    err = clEnqueueWriteBuffer(queue, changed_buffer2, CL_TRUE, 0, sizeof(int), &changed, 0, NULL, NULL);
    check (err, "Failed to write changed");

    // Set kernel arguments
    err = 0;
    err |= clSetKernelArg (compute_kernel, 0, sizeof (cl_mem), &next_buffer);
    err |= clSetKernelArg (compute_kernel, 1, sizeof (cl_mem), &cur_buffer);
    err |= clSetKernelArg (compute_kernel, 2, sizeof (cl_mem), &changed_buffer2);
    err |= clSetKernelArg (compute_kernel, 3, sizeof (cl_mem), &mask_buffer);
    check (err, "Failed to set kernel arguments");

    err = clEnqueueNDRangeKernel (queue, compute_kernel, 2, NULL, global, local, 0, NULL, NULL);
    check (err, "Failed to execute kernel");

    // Reading from GPU memory (does the tile changed ?)
    err = clEnqueueReadBuffer (queue, changed_buffer2, CL_TRUE, 0, sizeof (int), &changed, 0, NULL, NULL);
    check (err, "Failed to read changed2");
  
    // If we're here, it means that the sandpile is stable, and that the result
    // is located at the GPU memory, so we store it in our table (CPU memory).
    if (changed == 0) {
      return it;
    }

    {
      cl_mem tmp  = cur_buffer;
      cur_buffer  = next_buffer;
      next_buffer = tmp;
    }

  }

  return 0;
}

