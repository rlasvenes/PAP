
#ifndef GRAPHICS_IS_DEF
#define GRAPHICS_IS_DEF

#ifdef ENABLE_SDL

#include <SDL.h>

#include "global.h"

extern unsigned WIN_WIDTH, WIN_HEIGHT;

void graphics_init (void);
void graphics_alloc_images (void);
void graphics_share_texture_buffers (void);
void graphics_refresh (void);
void graphics_dump_image_to_file (char *filename);
void graphics_save_thumbnail (unsigned iteration);
int graphics_get_event (SDL_Event *event, int pause);
void graphics_clean (void);

#endif

#endif
