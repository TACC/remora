/*
  File: collect_power.c
*/

#include <dirent.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PATH_BUFFER_SIZE 360
#define ENERGY_MAX 265438953472

int main(int argc, char *argv[])
{
  if (argc != 4) {
    fprintf(stderr, "Usage: %s %s %s %s\n", argv[0], "node", "outdir", "tmpdir");
    return EXIT_FAILURE;
  }

  // Determine n_sockets

  DIR *dir = NULL;
  const char *dir_path = "/sys/devices/virtual/powercap/intel-rapl";

  dir = opendir(dir_path);
  if (dir == NULL) {
    printf("cannot open %s\n", dir_path);
    return EXIT_FAILURE;
  }

  int n_sockets = 0;
  struct dirent *ent;

  while ((ent = readdir(dir)) != NULL) {
    if (ent->d_type == DT_DIR && strncmp(ent->d_name, "intel-rapl:", 11) == 0)
      n_sockets++;
  }

  unsigned long long * energy_list_new = malloc(sizeof(unsigned long long int)*n_sockets);
  unsigned long long * energy_list_old = malloc(sizeof(unsigned long long int)*n_sockets);
  unsigned long long * energy_list_diff = malloc(sizeof(unsigned long long int)*n_sockets);
  unsigned int       * energy_list_overflow = malloc(sizeof(unsigned int)*n_sockets);

  for (int i = 0; i < n_sockets; i++)
    energy_list_overflow[i] = 0;

  if (dir != NULL)
    closedir(dir);

  // Previous data

  char *line_buf = NULL;
  size_t line_buf_size = 0;

  FILE *file;
  file = fopen("/dev/shm/remora_power.tmp","r");

  bool is_tmp_file_exists = false;
  int n_nodes = 0;
  char *rest;
  char *time_str;
  double time_old;

  if (file != NULL)  {
    is_tmp_file_exists = true;
    while (getline(&line_buf, &line_buf_size, file) >= 0) {
      rest = line_buf;
      time_str = strsep(&rest, " \t\n\v\f\r");
      time_old = atof(time_str);
    }

    int idx = 0;

    while (true)  {
      char *end = NULL;
      unsigned long long val;

      val = strtoull(rest, &end, 0);
      if (rest == end)
          break;
      else {
        if (idx < n_sockets)
          energy_list_old[idx] = val;
        else
          energy_list_overflow[idx] = (unsigned int)(val);
        idx++;
      }
      rest = end;
    }
    fclose(file);
  }

  // New data 

  char path[PATH_BUFFER_SIZE];
  struct timespec now;

  clock_gettime(CLOCK_REALTIME, &now);

  for (int socket = 0; socket < n_sockets; socket++)  {

    snprintf(path, sizeof(path), "%s/intel-rapl:%d/energy_uj", dir_path, socket);

    file = NULL;
  
    file = fopen(path, "r");

    if (file == NULL) {
      fprintf(stderr, "Cannot open %s\n", path);
      return EXIT_FAILURE;
    }

    if (getline(&line_buf, &line_buf_size, file) <= 0) {
      fprintf(stderr, "Failed reading %s\n", path);
      fclose(file);
      return EXIT_FAILURE;
    }

    energy_list_new[socket] = strtol(line_buf, NULL, 10);

    if (energy_list_new[socket] >= energy_list_old[socket])
      energy_list_diff[socket] = energy_list_new[socket] - energy_list_old[socket];
    else {
      energy_list_diff[socket] = energy_list_new[socket] - energy_list_old[socket] + ENERGY_MAX;
      energy_list_overflow[socket] += 1;
    }
    
    //if (energy_list_diff[socket] < 0)
      //energy_list_diff[socket] += 265438953472;

    if (file != NULL)
      fclose(file);
  }

  free(line_buf);

  double time_new = ((double)now.tv_sec*1e9 + now.tv_nsec) / 1000000000;

  for (int i = 0; i < n_sockets; i++)
    energy_list_diff[i] /= (time_new - time_old);

  file = fopen("/dev/shm/remora_power.tmp","w");
  fprintf(file, "%.9f ", time_new);

  for (int i = 0; i < n_sockets; i++) {
    fprintf(file, "%llu ", energy_list_new[i]);
  }
  for (int i = 0; i < n_sockets; i++) {
    fprintf(file, "%u", energy_list_overflow[i]);
    if (i <= n_sockets-2)
      fprintf(file, " ");
    else
      fprintf(file, "\n");
  }

  fclose(file);

  // Write data

  if (is_tmp_file_exists)  {

    char output_path[PATH_BUFFER_SIZE];

    // Energy 

    snprintf(output_path, sizeof(output_path), "%s/energy_%s.txt", argv[3], argv[1]);
    file = fopen(output_path, "a+");

    fprintf(file, "%.6f ", time_new);

    for (int i = 0; i < n_sockets; i++)  {
      fprintf(file, "%llu", energy_list_new[i] + energy_list_overflow[i] * ENERGY_MAX);
      if (i == n_sockets - 1)
        fprintf(file, "\n");
      else
        fprintf(file, " ");
    }
    fclose(file);

    // Power

    snprintf(output_path, sizeof(output_path), "%s/power_%s.txt", argv[3], argv[1]);
    file = fopen(output_path, "a+");

    fprintf(file, "%.6f ", 0.5 * (time_new + time_old));

    for (int i = 0; i < n_sockets; i++)  {
      fprintf(file, "%llu", energy_list_diff[i]);
      if (i == n_sockets - 1)
        fprintf(file, "\n");
      else
        fprintf(file, " ");
    }
    fclose(file);
  }
  free(energy_list_old);
  free(energy_list_new);
  free(energy_list_diff);
  free(energy_list_overflow);

  return EXIT_SUCCESS;
}

