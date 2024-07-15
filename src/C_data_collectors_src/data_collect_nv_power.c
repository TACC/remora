/*
  File: collect_nv_power.c
*/

#include <dirent.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PATH_BUFFER_SIZE 360

int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    fprintf(stderr, "Usage: %s %s %s %s\n", argv[0], "node", "outdir", "tmpdir");
    return EXIT_FAILURE;
  }

  // Determine n_sensors

  DIR *dir = NULL;
  const char *dir_path = "/sys/class/hwmon";

  dir = opendir(dir_path);
  if (dir == NULL)
  {
    printf("cannot open %s\n", dir_path);
    return EXIT_FAILURE;
  }

  int n_sensors = 0;
  int n_hwmon = 0;
  int n_init_index;
  struct dirent *ent;

  while ((ent = readdir(dir)) != NULL)
  {
    if ((ent->d_type == DT_DIR || ent->d_type == DT_LNK) && strncmp(ent->d_name, "hwmon", 5) == 0)
      n_hwmon++;
  }

  if (n_hwmon == 6) // Grace-Hopper node
  {
    n_init_index = 1;
    n_sensors = 4;
  }
  else if (n_hwmon == 8) // Grace-Grace node
  {
    n_init_index = 2;
    n_sensors = 6;
  }
  else
  {
    printf("Unrecognized node (neither GG nor HG)\n");
    return EXIT_FAILURE;
  }

  unsigned long long *power_list = malloc(sizeof(unsigned long long int) * n_sensors);

  if (dir != NULL)
    closedir(dir);

  // Previous data

  char *line_buf = NULL;
  size_t line_buf_size = 0;

  FILE *file;

  // New data

  char path[PATH_BUFFER_SIZE];
  struct timespec now;

  clock_gettime(CLOCK_REALTIME, &now);

  double time_new = ((double)now.tv_sec * 1e9 + now.tv_nsec) / 1000000000;

  for (int sensor = 0; sensor < n_sensors; sensor++)
  {

    snprintf(path, sizeof(path), "%s/hwmon%d/device/power1_average", dir_path, sensor + n_init_index);

    file = NULL;

    file = fopen(path, "r");

    if (file == NULL)
    {
      fprintf(stderr, "Cannot open %s\n", path);
      return EXIT_FAILURE;
    }

    if (getline(&line_buf, &line_buf_size, file) <= 0)
    {
      fprintf(stderr, "Failed reading %s\n", path);
      fclose(file);
      return EXIT_FAILURE;
    }

    power_list[sensor] = strtol(line_buf, NULL, 10);

    if (file != NULL)
      fclose(file);
  }

  free(line_buf);

  // Write data

  char output_path[PATH_BUFFER_SIZE];

  snprintf(output_path, sizeof(output_path), "%s/nv_power_%s.txt", argv[3], argv[1]);
  file = fopen(output_path, "a+");

  fprintf(file, "%.6f ", time_new);

  for (int i = 0; i < n_sensors; i++)
  {
    fprintf(file, "%.3f", power_list[i] / 1000000.0);
    if (i == n_sensors - 1)
      fprintf(file, "\n");
    else
      fprintf(file, " ");
  }
  fclose(file);

  free(power_list);

  return EXIT_SUCCESS;
}
