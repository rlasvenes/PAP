
#include "graphics.h"
#include "constants.h"
#include "debug.h"
#include "error.h"
#include "global.h"
#include "gmonitor.h"
#include "hooks.h"
#include "minmax.h"
#include "ocl.h"

#include <SDL_image.h>
#include <SDL_opengl.h>
#include <assert.h>
#include <sys/mman.h>

unsigned WIN_WIDTH  = 1024;
unsigned WIN_HEIGHT = 1024;

static SDL_Surface *temporary_surface = NULL;

static SDL_Window *win         = NULL;
static SDL_Renderer *ren       = NULL;
static SDL_Surface *surface[2] = {NULL, NULL};
static SDL_Texture *texture    = NULL;

#define THUMBNAILS_SIZE 512
static SDL_Surface *mini_surface = NULL;

static void graphics_create_surface (void)
{
  Uint32 rmask, gmask, bmask, amask;

  rmask = 0xff000000;
  gmask = 0x00ff0000;
  bmask = 0x0000ff00;
  amask = 0x000000ff;

  surface[0] = SDL_CreateRGBSurfaceFrom (
      image, DIM, DIM, 32, DIM * sizeof (Uint32), rmask, gmask, bmask, amask);
  if (surface[0] == NULL)
    exit_with_error ("SDL_CreateRGBSurfaceFrom failed (%s)", SDL_GetError ());

  surface[1] =
      SDL_CreateRGBSurfaceFrom (alt_image, DIM, DIM, 32, DIM * sizeof (Uint32),
                                rmask, gmask, bmask, amask);
  if (surface[1] == NULL)
    exit_with_error ("SDL_CreateRGBSurfaceFrom failed (%s)", SDL_GetError ());

  mini_surface =
      SDL_CreateRGBSurface (0, THUMBNAILS_SIZE, THUMBNAILS_SIZE, 32, 0xff000000,
                            0x00ff0000, 0x0000ff00, 0x000000ff);
  if (mini_surface == NULL)
    exit_with_error ("SDL_CreateRGBSurface failed (%s)", SDL_GetError ());
}

static SDL_Surface *more_recent_surface (void)
{
  SDL_Surface *s = NULL;

  for (int i = 0; i < 2; i++)
    if (surface[i]->pixels == image) {
      s = surface[i];
      // printf ("More recent surface : %d\n", i);
      break;
    }

  return s;
}

static void graphics_preload_surface (char *filename)
{
  unsigned size;

  // Chargement de l'image dans une surface temporaire
  temporary_surface = IMG_Load (filename);
  if (temporary_surface == NULL)
    exit_with_error ("IMG_Load (\"%s\") failed (%s)", filename,
                     SDL_GetError ());

  size = min (temporary_surface->w, temporary_surface->h);
  if (DIM)
    DIM = min (DIM, size);
  else
    DIM = size;
}

static void graphics_image_clean (void)
{
  // FIXME : est-ce vraiment nécessaire ?
  // Nettoyage de la transparence
  for (int i = 0; i < DIM; i++)
    for (int j = 0; j < DIM; j++)
      if ((cur_img (i, j) & 0xFF) == 0)
        // Si la composante alpha est nulle, on met l'ensemble du pixel à zéro
        cur_img (i, j) = 0;
      else
        cur_img (i, j) |= 0xFF;
}

void graphics_init (void)
{
  Uint32 render_flags =
      SDL_RENDERER_ACCELERATED | (vsync ? SDL_RENDERER_PRESENTVSYNC : 0);

  // Initialisation de SDL
  if (easypap_image_file != NULL || do_display)
    if (SDL_Init (SDL_INIT_VIDEO) != 0)
      exit_with_error ("SDL_Init failed (%s)", SDL_GetError ());

  if (do_display) {
    char title[1024];
    int x = SDL_WINDOWPOS_CENTERED;
    int y = SDL_WINDOWPOS_CENTERED;

    if (easypap_mpirun) {
      sprintf (
          title, "EasyPAP -- Process: [%d/%d]   Kernel: [%s]   Variant: [%s]",
          easypap_mpi_rank (), easypap_mpi_size (), kernel_name, variant_name);

      if (easypap_mpi_size () > 1 && debug_enabled ('M')) {
        WIN_WIDTH = WIN_HEIGHT = 512;
        x = (easypap_mpi_rank () % 2) * (WIN_WIDTH + 352 * 2);
        y = (easypap_mpi_rank () / 2) * (WIN_HEIGHT + 22) + 45;
      }
    } else
      sprintf (title, "EasyPAP -- Kernel: [%s]   Variant: [%s]", kernel_name,
               variant_name);

    // Création de la fenêtre sur l'écran
    win =
        SDL_CreateWindow (title, x, y, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN);
    if (win == NULL)
      exit_with_error ("SDL_CreateWindow failed (%s)", SDL_GetError ());

    unsigned drivers     = SDL_GetNumRenderDrivers ();
    int choosen_renderer = -1;

    for (int d = 0; d < drivers; d++) {
      SDL_RendererInfo info;
      SDL_GetRenderDriverInfo (d, &info);
      PRINT_DEBUG ('g', "Available Renderer %d: [%s]\n", d, info.name);
#ifdef __APPLE__
      if (opencl_used && !strcmp (info.name, "opengl"))
        choosen_renderer = d;
#endif
    }

    // Initialisation du moteur de rendu
    ren = SDL_CreateRenderer (win, choosen_renderer, render_flags);
    if (ren == NULL)
      exit_with_error ("SDL_CreateRenderer failed (%s)", SDL_GetError ());

    SDL_RendererInfo info;
    SDL_GetRendererInfo (ren, &info);
    PRINT_DEBUG ('g', "Main window renderer used: [%s]\n", info.name);
  }

  if (easypap_image_file != NULL)
    graphics_preload_surface (easypap_image_file);
  else if (!DIM)
    DIM = DEFAULT_DIM;

#ifdef ENABLE_MONITORING
  if (do_gmonitor) {
    int x = -1, y = -1, w = 0;

    SDL_GetWindowPosition (win, &x, &y);
    SDL_GetWindowSize (win, &w, NULL);

    gmonitor_init (x + w, y);

    SDL_RaiseWindow (win);
  }
#endif

  // Création d'une texture de taille DIM x DIM
  texture = SDL_CreateTexture (
      ren, SDL_PIXELFORMAT_RGBA8888, // SDL_PIXELFORMAT_RGBA32,
      SDL_TEXTUREACCESS_STATIC, DIM, DIM);

  PRINT_DEBUG ('i', "Init phase 1: SDL initialized (DIM = %d)\n", DIM);
}

void graphics_alloc_images (void)
{
  graphics_create_surface ();

  // Check if an image was preloaded
  if (temporary_surface != NULL) {
    // copie de temporary_surface vers surface
    {
      SDL_Rect src;

      src.x = 0;
      src.y = 0;
      src.w = DIM;
      src.h = DIM;

      SDL_BlitSurface (temporary_surface, /* src */
                       &src, surface[0],  /* dest */
                       NULL);
    }

    SDL_FreeSurface (temporary_surface);
    temporary_surface = NULL;

    graphics_image_clean ();
  }
}

void graphics_share_texture_buffers (void)
{
  GLuint texid;

  if (SDL_GL_BindTexture (texture, NULL, NULL) < 0)
    fprintf (stderr, "Warning: SDL_GL_BindTexture failed: %s\n",
             SDL_GetError ());

  glGetIntegerv (GL_TEXTURE_BINDING_2D, (GLint *)&texid);

  ocl_map_textures (texid);
}

void graphics_render_image (void)
{
  SDL_Rect src, dst;

  // Refresh texture
  if (opencl_used) {

    glFinish ();
    ocl_update_texture ();

  } else
    SDL_UpdateTexture (texture, NULL, image, DIM * sizeof (Uint32));

  src.x = 0;
  src.y = 0;
  src.w = DIM;
  src.h = DIM;

  // On redimensionne l'image pour qu'elle occupe toute la fenêtre
  dst.x = 0;
  dst.y = 0;
  dst.w = WIN_WIDTH;
  dst.h = WIN_HEIGHT;

  SDL_RenderCopy (ren, texture, &src, &dst);
}

void graphics_refresh (void)
{
  // On efface la scène dans le moteur de rendu (inutile !)
  SDL_RenderClear (ren);

  // On réaffiche l'image
  graphics_render_image ();

  // Met à jour l'affichage sur écran
  SDL_RenderPresent (ren);
}

void graphics_dump_image_to_file (char *filename)
{
  int r = IMG_SavePNG (more_recent_surface (), filename);

  if (r != 0)
    exit_with_error ("IMG_SavePNG (\"%s\") failed (%s)", filename,
                     SDL_GetError ());
}

void graphics_save_thumbnail (unsigned iteration)
{
  char filename[1024];

  SDL_Surface *s = more_recent_surface ();

  sprintf (filename, "./traces/data/thumb_%04d.png", iteration);

  SDL_FillRect (mini_surface, NULL, 0);

  SDL_SetSurfaceAlphaMod (s, 170);

  SDL_BlitScaled (s,                  /* src */
                  NULL, mini_surface, /* dest */
                  NULL);

  SDL_SetSurfaceAlphaMod (s, 255);

  int r = IMG_SavePNG (mini_surface, filename);

  if (r != 0)
    exit_with_error ("IMG_SavePNG (\"%s\") failed (%s)", filename,
                     SDL_GetError ());
}

int graphics_get_event (SDL_Event *event, int pause)
{
  if (pause)
    return SDL_WaitEvent (event);
  else
    return SDL_PollEvent (event);
}

void graphics_clean (void)
{
#ifdef ENABLE_MONITORING
  if (do_gmonitor)
    gmonitor_clean ();
#endif

  if (do_display) {

    if (ren != NULL) {
      SDL_DestroyRenderer (ren);
      ren = NULL;
    } else
      return;

    if (win != NULL) {
      SDL_DestroyWindow (win);
      win = NULL;
    } else
      return;
  }

  for (int i = 0; i < 2; i++)
    if (surface[i] != NULL) {
      SDL_FreeSurface (surface[i]);
      surface[i] = NULL;
    }

  if (do_display) {
    if (texture != NULL) {
      SDL_DestroyTexture (texture);
      texture = NULL;
    }

    IMG_Quit ();
    SDL_Quit ();
  }
}
