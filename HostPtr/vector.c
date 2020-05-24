
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#ifdef __APPLE__
#  include <OpenCL/opencl.h>
#else
#  include <CL/opencl.h>
#endif
#include "util.h"

#define MAX_PLATFORMS 3
#define MAX_DEVICES   5

#define ITERATIONS    100

// Application-specific data
//
#define KERNEL_NAME  "initialize"

unsigned SIZE = 4 * 1024 * 1024;
unsigned TILE = 128;

unsigned map_memory = 1;

unsigned *host_data = NULL;

cl_mem device_buffer;


static void alloc_buffers_and_user_data (cl_context context)
{
  // CPU side
  host_data = malloc (SIZE * sizeof(unsigned));

  if (map_memory)
    device_buffer = clCreateBuffer (context, CL_MEM_USE_HOST_PTR, sizeof(unsigned) * SIZE, host_data, NULL);
  else
    device_buffer = clCreateBuffer (context, CL_MEM_READ_WRITE,   sizeof(unsigned) * SIZE, NULL,      NULL);
  
  if (!device_buffer)
    error ("Failed to allocate device buffer");
}


static void check_output_data (void)
{
  unsigned correct = 0;

  for(int i = 0; i < SIZE; i++) {

    if (host_data [i] == i + 1)
      correct++;
  }
  printf("\tComputed '%d/%d' correct values!\n", correct, SIZE);
}


static void free_buffers_and_user_data (void)
{
  free (host_data);

  clReleaseMemObject (device_buffer);
}

static void release_output (cl_command_queue queue)
{
  if (map_memory) {
    cl_event prof_event;
    cl_ulong start, end;
    float elapsed_time;
    cl_int err;

    err = clEnqueueUnmapMemObject (queue,
				   device_buffer, host_data,
				   0, NULL,
				   &prof_event);

    check (err, "Failed to unmap buffer");
    
    clGetEventProfilingInfo (prof_event, CL_PROFILING_COMMAND_START,
			     sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo (prof_event, CL_PROFILING_COMMAND_END,
			     sizeof(cl_ulong), &end, NULL);
    elapsed_time = (end - start) * 1.0e-3f;
    printf ("\tUnmap Buffer performed in %lf µs\n",
	    elapsed_time);
  }
}

static void retrieve_output (cl_command_queue queue)
{
  cl_int err;
  cl_event prof_event;
  cl_ulong start, end;
  float elapsed_time;

  if (map_memory) {
    // If host data is mapped into device memory, then we must flush modifications back to host memory
    //
    void *ptr = clEnqueueMapBuffer (queue, device_buffer, CL_TRUE, CL_MAP_READ, 0, sizeof(unsigned) * SIZE, 0,
				    NULL, &prof_event, &err);
    check (err, "Failed to map buffer");
  
    if (ptr != host_data)
      printf ("Warning: mapped address = %p   !=   host_data = %p\n", ptr, host_data);
  } else {
    err = clEnqueueReadBuffer (queue, device_buffer, CL_TRUE, 0,
			       sizeof (unsigned) * SIZE, host_data, 0, NULL, &prof_event);
    check (err, "Failed to read from device buffer");
  }
  
  clGetEventProfilingInfo (prof_event, CL_PROFILING_COMMAND_START,
			   sizeof(cl_ulong), &start, NULL);
  clGetEventProfilingInfo (prof_event, CL_PROFILING_COMMAND_END,
			   sizeof(cl_ulong), &end, NULL);
  elapsed_time = (end - start) * 1.0e-3f;
  printf ("\tRead/Map Buffer performed in %lf µs\n",
	  elapsed_time);
}


int main (int argc, char** argv)
{
  cl_platform_id pf [MAX_PLATFORMS];
  cl_uint nb_platforms = 0;
  cl_int err;
  cl_device_type device_type = CL_DEVICE_TYPE_ALL;

  // Filter args
  //
  argv++;
  while (argc > 1) {
    if (!strcmp (*argv, "-g") || !strcmp (*argv, "--gpu-only")) {
      if (device_type != CL_DEVICE_TYPE_ALL)
	error ("--gpu-only and --cpu-only can not be specified at the same time\n");
      device_type = CL_DEVICE_TYPE_GPU;
    } else if (!strcmp (*argv, "-c") || !strcmp (*argv, "--cpu-only")) {
      if(device_type != CL_DEVICE_TYPE_ALL)
	error ("--gpu-only and --cpu-only can not be specified at the same time\n");
      device_type = CL_DEVICE_TYPE_CPU;
    } else if (!strcmp (*argv, "-n") || !strcmp (*argv, "--no-map")) {
	map_memory = 0;
    } else if (!strcmp (*argv, "-s") || !strcmp (*argv, "--size")) {
      unsigned i;
      int r;
      char c;

      r = sscanf (argv[1], "%u%[mMkK]", &SIZE, &c);

      if (r == 2) {
	if (c == 'k' || c == 'K')
	  SIZE *= 1024;
	else if (c == 'm' || c == 'M')
	  SIZE *= 1024 * 1024;
      }

      argc--; argv++;
    } else
      break;
    argc--; argv++;
  }

  if(argc > 1)
    TILE = atoi (*argv);

  // Get list of detected OpenCL platforms
  //
  err = clGetPlatformIDs (3, pf, &nb_platforms);
  check (err, "Failed to get platform IDs");

  printf ("%d OpenCL platforms detected\n", nb_platforms);

  // For each platform do
  //
  for (cl_int p = 0; p < nb_platforms; p++) {
    cl_uint num;
    int platform_valid = 1;
    char name [1024], vendor [1024];
    cl_device_id devices [MAX_DEVICES];
    cl_uint nb_devices = 0;
    cl_context context;                 // compute context
    cl_program program;                 // compute program
    cl_kernel kernel;

    err = clGetPlatformInfo (pf [p], CL_PLATFORM_NAME, 1024, name, NULL);
    check (err, "Failed to get Platform Info");

    err = clGetPlatformInfo (pf [p], CL_PLATFORM_VENDOR, 1024, vendor, NULL);
    check (err, "Failed to get Platform Info");

    printf ("Platform %d: %s - %s\n", p, name, vendor);

    // Get list of devices
    //
    err = clGetDeviceIDs (pf [p], device_type, MAX_DEVICES, devices, &nb_devices);
    printf ("nb devices = %d\n", nb_devices);

    if (nb_devices == 0)
      continue;

    // Create compute context with "device_type" devices
    //
    context = clCreateContext (0, nb_devices, devices, NULL, NULL, &err);
    check (err, "Failed to create compute context");

    // Load program source into memory
    //
    const char	*opencl_prog;
    opencl_prog = file_load (KERNEL_FILE);

    // Attach program source to context
    //
    program = clCreateProgramWithSource (context, 1, &opencl_prog, NULL, &err);
    check (err, "Failed to create program");

    // Compile program
    //
    {
      char flags [1024];

      sprintf (flags,
	       "-cl-mad-enable -cl-fast-relaxed-math -DSIZE=%d -DTILE=%d -DTYPE=%s",
	       SIZE, TILE, "float");

      err = clBuildProgram (program, 0, NULL, flags, NULL, NULL);
      if(err != CL_SUCCESS) {
	size_t len;

	// Display compiler log
	//
	clGetProgramBuildInfo (program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
	{
	  char buffer[len+1];

	  fprintf (stderr, "--- Compiler log ---\n");
	  clGetProgramBuildInfo (program, devices[0], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
	  fprintf (stderr, "%s\n", buffer);
	  fprintf (stderr, "--------------------\n");
	}
	if(err != CL_SUCCESS)
	  error ("Failed to build program!\n");
      }
    }

    // Create the compute kernel in the program we wish to run
    //
    kernel = clCreateKernel (program, KERNEL_NAME, &err);
    check (err, "Failed to create compute kernel");

    // Allocate and initialize input data
    //
    alloc_buffers_and_user_data (context);

    // Iterate over devices
    //
    for(cl_int dev = 0; dev < nb_devices; dev++) {
      cl_command_queue queue;

      char name [1024];
      cl_device_type dtype;

      err = clGetDeviceInfo (devices [dev], CL_DEVICE_NAME, 1024, name, NULL);
      check (err, "Cannot get type of device");
      err = clGetDeviceInfo (devices [dev], CL_DEVICE_TYPE, sizeof(cl_device_type), &dtype, NULL);
      check (err, "Cannot get type of device");

      printf ("* Device %d : %s [%s]\n", dev, (dtype == CL_DEVICE_TYPE_GPU) ? "GPU" : "CPU", name);

      // Query MAX_WORK_GROUP_SIZE
      //
      size_t max_workgroup_size;
      err = clGetDeviceInfo (devices [dev], CL_DEVICE_MAX_WORK_GROUP_SIZE,
			     sizeof(size_t), &max_workgroup_size, NULL);
      check (err, "Cannot get max workgroup size");

      printf ("\tMax Workgroup size = %zu\n", max_workgroup_size);

      // Create a command queue
      //
      queue = clCreateCommandQueue (context, devices[dev], CL_QUEUE_PROFILING_ENABLE, &err);
      check (err,"Failed to create command queue");

      // Initialize data in host memory
      //      
      for (int i = 0; i < SIZE; i++)
	host_data [i] = 0;

      // Execute kernel
      //
      {
	cl_event prof_event;
	cl_ulong start, end;
	float elapsed_time;
	struct timeval t1, t2;
	double timeInMicroseconds;
	size_t global[1] = { SIZE };    // global domain size for our calculation
	size_t local[1]  = { TILE };    // local domain size for our calculation

	printf ("\t%s\n", map_memory ? "Using Mapped Host Memory" : "Using Device Memory");
	printf ("\t%zu Threads in workgroups of size %zu\n", global[0], local[0]);

	// Set kernel arguments
	//
	err = 0;
	err |= clSetKernelArg (kernel, 0, sizeof(cl_mem), &device_buffer);
	check (err, "Failed to set kernel arguments");

	gettimeofday (&t1, NULL);

	for (unsigned iter = 0; iter < ITERATIONS; iter++) {
	  err = clEnqueueNDRangeKernel (queue, kernel, 1, NULL, global, local,
				        0, NULL, &prof_event);
	  check (err, "Failed to execute kernel");
	}

	// Wait for the command commands to get serviced before reading back results
	//
	clFinish (queue);
	
	gettimeofday (&t2,NULL);
	
	// Display performance stats
	//
	timeInMicroseconds = (double) TIME_DIFF(t1, t2) / ITERATIONS;
      
	printf ("\tComputation performed in %lf µs over device #%d\n",
		timeInMicroseconds,
		dev);

        clGetEventProfilingInfo (prof_event, CL_PROFILING_COMMAND_START,
			        sizeof(cl_ulong), &start, NULL);
        clGetEventProfilingInfo (prof_event, CL_PROFILING_COMMAND_END,
			        sizeof(cl_ulong), &end, NULL);
        elapsed_time = (end - start) * 1.0e-3f;
	printf ("\tOpenCL reports %lf µs for last iteration\n",
		elapsed_time),

	printf ("\tKernel bandwidth (based on OpenCL) : %f GB/s\n",
		(SIZE * sizeof(unsigned)) / (1024.0 * 1024 * 1024) / (elapsed_time * 1e-6));
	clReleaseEvent (prof_event);
      }

      // Retrieve host_buffer modifications
      //
      retrieve_output (queue);

      // Validate computation
      //
      check_output_data ();

      release_output (queue);
      
      clReleaseCommandQueue (queue);
    }

    // Cleanup
    //
    free_buffers_and_user_data ();

    clReleaseKernel (kernel);
    clReleaseProgram (program);
    clReleaseContext (context);
  }

  return 0;
}
