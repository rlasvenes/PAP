#include <SDL_image.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "graphics.h"
#include "cpustat.h"
#include "debug.h"
#include "error.h"
#include "global.h"
#include "gmonitor.h"
#include "monitoring.h"
#include "trace_common.h"

unsigned do_gmonitor = 0;

static unsigned MONITOR_WIDTH  = 0;
static unsigned MONITOR_HEIGHT = 0;

static unsigned NBCORES      = 1;

static SDL_Window *win      = NULL;
static SDL_Renderer *ren    = NULL;
static SDL_Texture *texture = NULL;

static Uint32 *restrict trace_img = NULL;

void gmonitor_init (int x, int y)
{
  NBCORES = easypap_requested_number_of_threads ();

  MONITOR_WIDTH  = 352;
  MONITOR_HEIGHT = 352;

  PRINT_DEBUG ('m', "Gmonitor window: %u x %u\n", MONITOR_WIDTH,
               MONITOR_HEIGHT);

  // Création de la fenêtre sur l'écran
  win = SDL_CreateWindow ("Tiling", x, y, MONITOR_WIDTH, MONITOR_HEIGHT,
                          SDL_WINDOW_SHOWN);
  if (win == NULL)
    exit_with_error ("SDL_CreateWindow failed (%s)", SDL_GetError ());

  // Initialisation du moteur de rendu
  ren = SDL_CreateRenderer (win, -1, SDL_RENDERER_ACCELERATED);
  if (ren == NULL)
    exit_with_error ("SDL_CreateRenderer failed (%s)", SDL_GetError ());

  SDL_RendererInfo info;
  SDL_GetRendererInfo (ren, &info);
  PRINT_DEBUG ('g', "Tiling window renderer: [%s]\n", info.name);

  // Creation d'une surface capable de mémoire quel processeur/thread a
  // travaillé sur quel pixel
  trace_img = malloc (DIM * DIM * sizeof (Uint32));
  bzero (trace_img, DIM * DIM * sizeof (Uint32));

  // Création d'une texture DIM x DIM sur la carte graphique
  texture = SDL_CreateTexture (
      ren, SDL_PIXELFORMAT_RGBA8888, // SDL_PIXELFORMAT_RGBA32,
      SDL_TEXTUREACCESS_STATIC, DIM, DIM);
  if (texture == NULL)
    exit_with_error ("SDL_CreateTexture failed (%s)", SDL_GetError ());

  if (TTF_Init () < 0)
    exit_with_error ("TTF_Init failed (%s)", TTF_GetError ());

  {
    int x = -1, y = -1, h = -1, w = -1;

    SDL_GetWindowPosition (win, &x, &y);
    SDL_GetWindowSize (win, &w, &h);

    if (easypap_mpi_size () > 1)
      cpustat_init (x + w, y);
    else
      cpustat_init (x, y + h + 22);
  }
}

void __gmonitor_start_iteration (long time)
{
  cpustat_reset (time);
}

void __gmonitor_start_tile (long time, int who)
{
  cpustat_start_work (time, who);
}

void __gmonitor_end_tile (long time, int who, int x, int y, int width,
                          int height)
{
  cpustat_finish_work (time, who);

  for (int i = y; i < y + height; i++)
    for (int j = x; j < x + width; j++)
      trace_img[i * DIM + j] = cpu_colors[who % MAX_COLORS];

  cpustat_start_idle (what_time_is_it (), who);
}

void __gmonitor_end_iteration (long time)
{
  cpustat_freeze (time);

  cpustat_display_stats ();

  SDL_Rect src, dst;

  SDL_UpdateTexture (texture, NULL, trace_img, DIM * sizeof (Uint32));

  src.x = 0;
  src.y = 0;
  src.w = DIM;
  src.h = DIM;

  // On redimensionne l'image pour qu'elle occupe toute la fenêtre
  dst.x = 0;
  dst.y = 0;
  dst.w = MONITOR_WIDTH;
  dst.h = MONITOR_HEIGHT;

  SDL_RenderClear (ren);

  SDL_RenderCopy (ren, texture, &src, &dst);

  SDL_RenderPresent (ren);

  bzero (trace_img, DIM * DIM * sizeof (Uint32));
}

void gmonitor_clean ()
{
  if (ren != NULL)
    SDL_DestroyRenderer (ren);
  else
    return;

  if (win != NULL)
    SDL_DestroyWindow (win);
  else
    return;

  if (trace_img != NULL)
    free (trace_img);

  if (texture != NULL)
    SDL_DestroyTexture (texture);

  cpustat_clean ();

  TTF_Quit ();
}
