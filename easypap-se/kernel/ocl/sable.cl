#include "kernel/ocl/common.cl"


__kernel void sable_ocl (__global unsigned *in, __global unsigned *out)
{
  int y = get_global_id (1);
  int x = get_global_id (0);

  __local unsigned tile [TILEY][TILEX];
  __local unsigned tile_tmp [TILEY][TILEX];
  unsigned xloc = get_local_id(0);
  unsigned yloc = get_local_id(1);

  tile[yloc][xloc] = in[y * DIM + x];
  tile_tmp[yloc][xloc] = in[y * DIM + x];

  barrier (CLK_LOCAL_MEM_FENCE);

  tile[yloc][xloc] = (tile_tmp[yloc][xloc] % 4) +
                     (tile_tmp[yloc][xloc-1] / 4) +
                     (tile_tmp[yloc][xloc+1] / 4) +
                     (tile_tmp[yloc-1][xloc] / 4) +
                     (tile_tmp[yloc+1][xloc] / 4);



  // in = écriture out = lecture
  out[y*DIM+x] = tile[yloc][xloc];
}

__kernel void sable_ocl_mine (__global unsigned *in, __global unsigned *out, __global int *changed)
{
  int x = get_global_id (0);
  int y = get_global_id (1);

  if ((x > 0 && x < DIM-1) && (y > 0 && y < DIM-1)) {
    int xloc = get_local_id(0);
    int yloc = get_local_id(1);

    int current = y*DIM+x;

    int left  = x > 0     ? in[current-1]   / 4 : 0;
    int right = x < DIM-2 ? in[current+1]   / 4 : 0;
    int up    = y > 0     ? in[current-DIM] / 4 : 0;
    int down  = y < DIM-2 ? in[current+DIM] / 4 : 0;
    int me    = in[current] % 4;

    int sum = left + right + up + down + me;

    out[current] = sum;
    unsigned cur = out[current];
    unsigned next = in[current];

    if (out[current] - in[current] != 0)    
      *changed = 1;
  }
}

__kernel void sable_ocl_opti (__global unsigned *in, __global unsigned *out, __global int *changed, __global unsigned *mask)
{
  int x = get_global_id (0);
  int y = get_global_id (1);

  if (mask[y * (DIM/TILEY) + x] == 1) {
    return;
  }
  
  if ((x > 0 && x < DIM-1) && (y > 0 && y < DIM-1)) {
    int xloc = get_local_id(0);
    int yloc = get_local_id(1);

    int current = y*DIM+x;

    int left  = x > 0     ? in[current-1]   / 4 : 0;
    int right = x < DIM-2 ? in[current+1]   / 4 : 0;
    int up    = y > 0     ? in[current-DIM] / 4 : 0;
    int down  = y < DIM-2 ? in[current+DIM] / 4 : 0;
    int me    = in[current] % 4;

    int sum = left + right + up + down + me;

    barrier(CLK_LOCAL_MEM_FENCE);

    out[current] = sum;
    unsigned cur = out[current];
    unsigned next = in[current];

    if (cur - next != 0)    
      *changed = 1;

    barrier(CLK_LOCAL_MEM_FENCE);

    mask[y * (DIM/TILEY) + x] = changed;
  }
}

// DO NOT MODIFY: this kernel updates the OpenGL texture buffer
// This is a sable-specific version (generic version is defined in common.cl)
__kernel void sable_update_texture (__global unsigned *cur, __write_only image2d_t tex)
{
  int y = get_global_id (1);
  int x = get_global_id (0);
  int2 pos = (int2)(x, y);
  unsigned c = cur [y * DIM + x];
  unsigned r = 0, v = 0, b = 0;

  if (c == 1)
    v = 255;
  else if (c == 2)
    b = 255;
  else if (c == 3)
    r = 255;
  else if (c == 4)
    r = v = b = 255;
  else if (c > 4)
    r = v = b = (2 * c);

  c = rgba(r, v, b, 0xFF);

  write_imagef (tex, pos, color_scatter (c));
}
