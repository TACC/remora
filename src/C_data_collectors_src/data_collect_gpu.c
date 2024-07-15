/*
  File: collect_gpu.c
*/

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "nvml.h"

#define PATH_BUFFER_SIZE 360

int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    fprintf(stderr, "Usage: %s %s %s %s\n", argv[0], "node", "outdir", "tmpdir");
    return EXIT_FAILURE;
  }

  int ndev;

  nvmlInit();

  if (nvmlDeviceGetCount(&ndev) != NVML_SUCCESS)
  {
    fprintf(stderr, "Failed reading device count\n");
    nvmlShutdown();
    return EXIT_FAILURE;
  }

  struct timespec now;

  clock_gettime(CLOCK_REALTIME, &now);

  double time_new = ((double)now.tv_sec * 1e9 + now.tv_nsec) / 1000000000;

  // Write data

  char output_path[PATH_BUFFER_SIZE];
  snprintf(output_path, sizeof(output_path), "%s/gpu_memory_stats_%s.txt", argv[3], argv[1]);
  FILE *file = fopen(output_path, "a+");

  fprintf(file, "%.3f ", time_new);

  for (int i = 0; i < ndev; i++)
  {

    nvmlDevice_t device;
    nvmlReturn_t ret;
    nvmlMemory_t memory;

    ret = nvmlDeviceGetHandleByIndex(i, &device);

    if (ret != NVML_SUCCESS)
    {
      fprintf(stderr, "Failed reading NVML device: %s\n", nvmlErrorString(ret));
      nvmlShutdown();
      return EXIT_FAILURE;
    }

    ret = nvmlDeviceGetMemoryInfo(device, &memory);

    if (ret != NVML_SUCCESS)
    {
      fprintf(stderr, "Failed reading NVML memory: %s\n", nvmlErrorString(ret));
      nvmlShutdown();
      return EXIT_FAILURE;
    }

    fprintf(file, "%.6f %.6f", memory.free / (1024.0 * 1024.0 * 1024.0), memory.used / (1024.0 * 1024.0 * 1024.0));

    if (i <= ndev - 2)
      fprintf(file, " ");
    else
      fprintf(file, "\n");
  }
  fclose(file);

out:
  nvmlShutdown();

  return EXIT_SUCCESS;
}
