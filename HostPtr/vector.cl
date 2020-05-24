
__kernel void initialize (__global unsigned *vec)
{
  int index = get_global_id (0);

   vec[index] = index + 1;
}
