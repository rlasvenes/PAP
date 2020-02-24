
#include "easypap.h"

#include <omp.h>
#include <stdbool.h>

#ifdef ENABLE_VECTO
#include <immintrin.h>
#endif

///////////////////////////// Simple sequential version (seq)
// Suggested cmdline:
// ./run --load-image images/shibuya.png --kernel rotation90 --pause
//
unsigned rotation90_compute_seq (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {

    for (int i = 0; i < DIM; i++)
      for (int j = 0; j < DIM; j++)
        next_img (DIM - i - 1, j) = cur_img (j, i);

    swap_images ();
  }

  return 0;
}

// Tile inner computation
static void do_tile_reg (int x, int y, int width, int height)
{
  #pragma omp parallel for collapse(2) schedule(dynamic)
  for (int i = y; i < y + height; i++)
    for (int j = x; j < x + width; j++) {
      next_img (DIM - i - 1, j) = cur_img (j, i);
    }
}

static inline void do_tile (int x, int y, int width, int height, int who)
{
  monitoring_start_tile (who);

  do_tile_reg (x, y, width, height);

  monitoring_end_tile (x, y, width, height, who);
}

unsigned rotation90_compute_tiled (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {
    #pragma omp parallel for collapse(2) schedule(runtime)
    for (int i = 0; i < DIM; i += TILE_SIZE)
      for (int j = 0; j < DIM; j += TILE_SIZE) {
        //#pragma omp master
        do_tile (i, j, TILE_SIZE, TILE_SIZE, omp_get_thread_num());
      }
    swap_images ();
  }

  return 0;
}
