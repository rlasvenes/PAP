
#include "easypap.h"

#include <omp.h>

///////////////////////////// Sequential version (tiled)
// Suggested cmdline(s):
// ./run -l images/1024.png -k blur -v seq
//
static void do_tile_reg (int x, int y, int width, int height)
{
  for (int i = y; i < y + height; i++)
    for (int j = x; j < x + width; j++) {
      unsigned r = 0, g = 0, b = 0, a = 0, n = 0;

      int i_d = (i > 0) ? i - 1 : i;
      int i_f = (i < DIM - 1) ? i + 1 : i;
      int j_d = (j > 0) ? j - 1 : j;
      int j_f = (j < DIM - 1) ? j + 1 : j;

      for (int yloc = i_d; yloc <= i_f; yloc++)
        for (int xloc = j_d; xloc <= j_f; xloc++) {
          unsigned c = cur_img (yloc, xloc);
          r += extract_red (c);
          g += extract_green (c);
          b += extract_blue (c);
          a += extract_alpha (c);
          n += 1;
        }

      r /= n;
      g /= n;
      b /= n;
      a /= n;

      next_img (i, j) = rgba (r, g, b, a);
    }
}

static void do_tile_reg_nocheck (int x, int y, int width, int height)
{
  for (int i = y; i < y + height; i++)
    for (int j = x; j < x + width; j++) {
      unsigned r = 0, g = 0, b = 0, a = 0, n = 9;

      /* int i_d = (i > 0) ? i - 1 : i;
      int i_f = (i < DIM - 1) ? i + 1 : i;
      int j_d = (j > 0) ? j - 1 : j;
      int j_f = (j < DIM - 1) ? j + 1 : j;
      */
      for (int yloc = i - 1; yloc <= i + 1; yloc++)
        for (int xloc = j - 1; xloc <= j + 1; xloc++) {
          unsigned c = cur_img (yloc, xloc);
          r += extract_red (c);
          g += extract_green (c);
          b += extract_blue (c);
          a += extract_alpha (c);
        }

      r /= n;
      g /= n;
      b /= n;
      a /= n;

      next_img (i, j) = rgba (r, g, b, a);
    }
}

unsigned blur_compute_seq (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {

    do_tile_reg (0, 0, DIM, DIM);

    swap_images ();
  }

  return 0;
}

///////////////////////////// Tiled sequential version (tiled)
// Suggested cmdline(s):
// ./run -l images/1024.png -k blur -v tiled -g 16 -m
//
static inline void do_tile (int x, int y, int width, int height, int who)
{
  monitoring_start_tile (who);

  do_tile_reg (x, y, width, height);

  monitoring_end_tile (x, y, width, height, who);
}

///////////////////////////// Tiled sequential version without check (tiled_check)
// Suggested cmdline(s):
// ./run -l images/1024.png -k blur -v tiled_nocheck -g 8 -m
// Do the same thing as "do_tile" function, but without checking the border
static inline void do_tile_nocheck (int x, int y, int width, int height, int who)
{
  monitoring_start_tile (who);

  do_tile_reg_nocheck (x, y, width, height);

  monitoring_end_tile (x, y, width, height, who);
}

unsigned blur_compute_tiled (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {


    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = 0; x < DIM; x += TILE_SIZE) {
        do_tile (x, y, TILE_SIZE, TILE_SIZE, 0);
      }
    swap_images ();
  }

  return 0;
}

unsigned blur_compute_tiled_nocheck (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {

    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = 0; x < DIM; x += TILE_SIZE) {
        do_tile_nocheck(x, y, TILE_SIZE, TILE_SIZE, omp_get_thread_num());
      }
    swap_images ();
  }

  return 0;
}

///////////////////////////// Optimized tiled sequential version (tiled_opt)
// Suggested cmdline(s):
// ./run -l images/1024.png -k blur -v tiled_opt -g 16 -m
//
unsigned blur_compute_tiled_opt (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {

    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = 0; x < DIM; x += TILE_SIZE) {
        // si on est sur un bord, on check
        int bord1 = ((y == 0)      && (x >= 0 && x < DIM)) || ((x + TILE_SIZE) % DIM == 0);
        int bord2 = ((y == DIM-1)  && (x >= 0 && x < DIM)) || ((x + TILE_SIZE) % DIM == 0);
        int bord3 = ((x == 0)      && (y >= 0 && y < DIM)) || ((y + TILE_SIZE) % DIM == 0);
        int bord4 = ((x == DIM-1)  && (y >= 0 && y < DIM)) || ((y + TILE_SIZE) % DIM == 0);

        if ( bord1 || bord2 || bord3 || bord4 ) {
          do_tile(x, y, TILE_SIZE, TILE_SIZE, omp_get_thread_num());
        } else { // milieu de l'image
          do_tile_nocheck(x, y, TILE_SIZE, TILE_SIZE, omp_get_thread_num());
        }
      }
    swap_images ();
  }

  return 0;
}

///////////////////////////// Optimized tiled parallel version (omp_tiled)
// Suggested cmdline(s):
// ./run -l images/1024.png -k blur -v omp_tiled -g 16 -m
//
unsigned blur_compute_omp_tiled (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {

    #pragma omp parallel for collapse(2) schedule(runtime)
    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = 0; x < DIM; x += TILE_SIZE) {
        // si on est sur un bord, on check
        int bord1 = ((y == 0)      && (x >= 0 && x < DIM)) || ((x + TILE_SIZE) % DIM == 0);
        int bord2 = ((y == DIM-1)  && (x >= 0 && x < DIM)) || ((x + TILE_SIZE) % DIM == 0);
        int bord3 = ((x == 0)      && (y >= 0 && y < DIM)) || ((y + TILE_SIZE) % DIM == 0);
        int bord4 = ((x == DIM-1)  && (y >= 0 && y < DIM)) || ((y + TILE_SIZE) % DIM == 0);
        
        if ( bord1 || bord2 || bord3 || bord4 ) {
          #pragma omp task
          do_tile(x, y, TILE_SIZE, TILE_SIZE, omp_get_thread_num());
        } else { // milieu de l'image
          #pragma omp task
          do_tile_nocheck(x, y, TILE_SIZE, TILE_SIZE, omp_get_thread_num());
        }
      }
    swap_images ();
  }

  return 0;
}
