/*
  File: collect_nv_temperature.c
*/

#include <dirent.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PATH_BUFFER_SIZE 1000

int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    fprintf(stderr, "Usage: %s %s %s %s\n", argv[0], "node", "outdir", "tmpdir");
    return EXIT_FAILURE;
  }

  // Determine n_zones

  DIR *dir = NULL;
  const char *dir_path = "/sys/devices/virtual/thermal/thermal_zone0/hwmon0";

  dir = opendir(dir_path);
  if (dir == NULL)
  {
    fprintf(stderr, "Cannot open %s\n", dir_path);
    return EXIT_FAILURE;
  }

  int n_zones = 0;
  char *line_buf = NULL;
  size_t line_buf_size = 0;

  int zone_idx;
  int *idx_list;
  unsigned long *temp_list;
  unsigned long long temp;

  struct timespec now;

  clock_gettime(CLOCK_REALTIME, &now);

  FILE *file;

  struct dirent *ent;

  while ((ent = readdir(dir)) != NULL)
  {
    if (ent->d_type != DT_DIR && strncmp(ent->d_name, "temp", 4) == 0 && strstr(ent->d_name, "_input") != NULL)
    {
      n_zones++;
    }
  }

  rewinddir(dir);

  temp_list = malloc(sizeof(unsigned long) * n_zones);
  idx_list = malloc(sizeof(int) * n_zones);
  n_zones = 0;

  double time_now = ((double)now.tv_sec * 1e9 + now.tv_nsec) / 1000000000;

  while ((ent = readdir(dir)) != NULL)
  {
    if (ent->d_type != DT_DIR && strncmp(ent->d_name, "temp", 4) == 0 && strstr(ent->d_name, "_input") != NULL)
    {
      char temp_file_path[PATH_BUFFER_SIZE];

      sscanf(ent->d_name, "temp%d_input", &zone_idx);
      snprintf(temp_file_path, sizeof(temp_file_path), "%s/%s", dir_path, ent->d_name);

      file = NULL;
      file = fopen(temp_file_path, "r");

      if (file == NULL)
      {
        fprintf(stderr, "Cannot open %s\n", temp_file_path);
        return EXIT_FAILURE;
      }

      if (getline(&line_buf, &line_buf_size, file) <= 0)
      {
        fprintf(stderr, "Failed reading file %s\n", temp_file_path);
        fclose(file);
        return EXIT_FAILURE;
      }

      temp = strtoul(line_buf, NULL, 10);

      idx_list[n_zones] = zone_idx;
      temp_list[n_zones] = temp;
      n_zones++;
    }
  }

  if (dir != NULL)
    closedir(dir);

  int swap_idx;
  unsigned long swap_temp;

  for (int i = 0; i < n_zones - 1; i++)
  {
    for (int j = 0; j < n_zones - i - 1; j++)
    {
      if (idx_list[j] > idx_list[j + 1])
      {
        swap_idx = idx_list[j];
        idx_list[j] = idx_list[j + 1];
        idx_list[j + 1] = swap_idx;
        swap_temp = temp_list[j];
        temp_list[j] = temp_list[j + 1];
        temp_list[j + 1] = swap_temp;
      }
    }
  }

  char output_path[PATH_BUFFER_SIZE];
  snprintf(output_path, sizeof(output_path), "%s/temperature_cpu_%s.txt", argv[3], argv[1]);
  file = fopen(output_path, "a+");

  fprintf(file, "%.3f ", time_now);

  for (int i = 0; i < n_zones; i++)
  {
    fprintf(file, "%lu ", temp_list[i]);
    if (i == n_zones - 1)
      fprintf(file, "\n");
    else
      fprintf(file, " ");
  }
  fclose(file);

  free(idx_list);
  free(temp_list);
  free(line_buf);

  return EXIT_SUCCESS;
}
